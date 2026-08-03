#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "app/authcontroller.h"
#include "app/session.h"
#include "app/thememanager.h"
#include "repo/triprepository.h"
#include "ui/logindialog.h"
#include "ui/settingsdialog.h"
#include "ui/statswidget.h"
#include "ui/tripcarddelegate.h"
#include "ui/tripeditdialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_tripRepo = new TripRepository;
    m_tripModel = new TripListModel(this);
    setupUI();
    reloadTrips();
}

MainWindow::~MainWindow()
{
    delete m_tripRepo;
}

void MainWindow::setupUI()
{
    setWindowTitle(QStringLiteral("TransitLog 行程记录"));
    resize(720, 560);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    setCentralWidget(central);

    // ---- 顶栏 ----
    auto* topBar = new QWidget(central);
    topBar->setObjectName(QStringLiteral("topBar"));
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 8, 16, 8);
    topLayout->setSpacing(8);

    auto* title = new QLabel(QStringLiteral("TransitLog"), topBar);
    title->setObjectName(QStringLiteral("appTitle"));
    topLayout->addWidget(title);
    topLayout->addSpacing(12);

    m_navTrips = new QPushButton(QStringLiteral("行程"), topBar);
    m_navTrips->setObjectName(QStringLiteral("navButton"));
    m_navTrips->setCheckable(true);
    m_navTrips->setChecked(true);
    m_navStats = new QPushButton(QStringLiteral("统计"), topBar);
    m_navStats->setObjectName(QStringLiteral("navButton"));
    m_navStats->setCheckable(true);
    topLayout->addWidget(m_navTrips);
    topLayout->addWidget(m_navStats);
    topLayout->addStretch();

    auto* themeBtn = new QPushButton(QStringLiteral("🌓 主题"), topBar);
    themeBtn->setObjectName(QStringLiteral("iconButton"));
    topLayout->addWidget(themeBtn);

    m_userLabel = new QLabel(Session::instance().currentUser().username, topBar);
    m_userLabel->setObjectName(QStringLiteral("hintLabel"));
    topLayout->addWidget(m_userLabel);

    auto* settingsBtn = new QPushButton(QStringLiteral("⚙ 设置"), topBar);
    settingsBtn->setObjectName(QStringLiteral("iconButton"));
    topLayout->addWidget(settingsBtn);

    root->addWidget(topBar);

    // ---- 页面 ----
    m_pages = new QStackedWidget(central);
    root->addWidget(m_pages, 1);

    // 行程页
    auto* tripPage = new QWidget(m_pages);
    auto* tripLayout = new QVBoxLayout(tripPage);
    tripLayout->setContentsMargins(20, 16, 20, 16);
    tripLayout->setSpacing(12);

    auto* header = new QHBoxLayout;
    auto* pageTitle = new QLabel(QStringLiteral("我的行程"), tripPage);
    pageTitle->setObjectName(QStringLiteral("pageTitle"));
    m_tripCountLabel = new QLabel(tripPage);
    m_tripCountLabel->setObjectName(QStringLiteral("hintLabel"));
    auto* addBtn = new QPushButton(QStringLiteral("＋ 记行程"), tripPage);
    addBtn->setObjectName(QStringLiteral("primaryButton"));
    header->addWidget(pageTitle);
    header->addWidget(m_tripCountLabel);
    header->addStretch();
    header->addWidget(addBtn);
    tripLayout->addLayout(header);

    m_emptyStack = new QStackedWidget(tripPage);
    auto* emptyPage = new QWidget(m_emptyStack);
    auto* emptyLayout = new QVBoxLayout(emptyPage);
    auto* emptyLabel = new QLabel(QStringLiteral("暂无行程\n点击右上角「＋ 记行程」开始记录"), emptyPage);
    emptyLabel->setObjectName(QStringLiteral("hintLabel"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyLabel);
    m_emptyStack->addWidget(emptyPage);

    m_tripList = new QListView(m_emptyStack);
    m_tripList->setModel(m_tripModel);
    m_tripList->setItemDelegate(new TripCardDelegate(m_tripList));
    m_tripList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tripList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tripList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tripList->viewport()->setMouseTracking(true);
    m_emptyStack->addWidget(m_tripList);
    tripLayout->addWidget(m_emptyStack, 1);

    m_pages->addWidget(tripPage);

    // 统计页
    m_statsWidget = new StatsWidget(m_pages);
    m_pages->addWidget(m_statsWidget);

    // ---- 连接 ----
    connect(m_navTrips, &QPushButton::clicked, this, &MainWindow::onShowTrips);
    connect(m_navStats, &QPushButton::clicked, this, &MainWindow::onShowStats);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddTrip);
    connect(themeBtn, &QPushButton::clicked, this, &MainWindow::onToggleTheme);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);

    connect(m_tripList, &QListView::doubleClicked, this, &MainWindow::onEditTrip);
    connect(m_tripList, &QWidget::customContextMenuRequested,
            this, &MainWindow::onShowContextMenu);

    auto* delShortcut = new QShortcut(QKeySequence::Delete, m_tripList);
    connect(delShortcut, &QShortcut::activated, this, &MainWindow::onDeleteSelected);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this](Theme t) {
                qApp->setStyleSheet(ThemeManager::instance().buildQSS(t));
                m_tripList->viewport()->update();
            });
}

void MainWindow::onShowTrips()
{
    m_pages->setCurrentIndex(0);
    m_navTrips->setChecked(true);
    m_navStats->setChecked(false);
}

void MainWindow::onShowStats()
{
    m_pages->setCurrentIndex(1);
    m_navStats->setChecked(true);
    m_navTrips->setChecked(false);
}

void MainWindow::reloadTrips()
{
    const QString uid = Session::instance().userId();
    const QList<TransportMode> modes = m_tripRepo->transportModes();
    const QList<TripTag> tags = m_tripRepo->tripTags();

    QHash<QString, TransportMode> modeMap;
    for (const TransportMode& m : modes)
        modeMap.insert(m.code, m);
    QHash<QString, TripTag> tagMap;
    for (const TripTag& t : tags)
        tagMap.insert(t.code, t);

    const QList<Trip> trips = m_tripRepo->tripsForUser(uid);

    m_tripModel->setModes(modeMap);
    m_tripModel->setTags(tagMap);
    m_tripModel->setTrips(trips);
    m_statsWidget->setModes(modeMap);
    m_statsWidget->setTrips(trips);

    m_tripCountLabel->setText(QStringLiteral("共 %1 段").arg(trips.size()));
    m_emptyStack->setCurrentIndex(trips.isEmpty() ? 0 : 1);
}

void MainWindow::onAddTrip()
{
    TripEditDialog dlg(m_tripRepo->transportModes(), m_tripRepo->tripTags(), Trip(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Trip t = dlg.trip();
    t.userId = Session::instance().userId();
    m_tripRepo->saveTrip(t);
    reloadTrips();
}

void MainWindow::onEditTrip(const QModelIndex& index)
{
    const Trip existing = m_tripModel->tripAt(index.row());
    if (existing.id.isEmpty())
        return;
    TripEditDialog dlg(m_tripRepo->transportModes(), m_tripRepo->tripTags(), existing, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Trip t = dlg.trip();
    t.userId = existing.userId;
    m_tripRepo->saveTrip(t);
    reloadTrips();
}

void MainWindow::onDeleteTrip(const QModelIndex& index)
{
    const Trip t = m_tripModel->tripAt(index.row());
    if (t.id.isEmpty())
        return;
    const auto ret = QMessageBox::question(this, QStringLiteral("删除行程"),
                                           QStringLiteral("确定删除这段行程？"));
    if (ret != QMessageBox::Yes)
        return;
    m_tripRepo->softDeleteTrip(t.id);
    reloadTrips();
}

void MainWindow::onDeleteSelected()
{
    const QModelIndex idx = m_tripList->currentIndex();
    if (idx.isValid())
        onDeleteTrip(idx);
}

void MainWindow::onShowContextMenu(const QPoint& pos)
{
    const QModelIndex idx = m_tripList->indexAt(pos);
    if (!idx.isValid())
        return;
    QMenu menu(this);
    QAction* edit = menu.addAction(QStringLiteral("编辑"));
    QAction* del = menu.addAction(QStringLiteral("删除"));
    QAction* chosen = menu.exec(m_tripList->viewport()->mapToGlobal(pos));
    if (chosen == edit)
        onEditTrip(idx);
    else if (chosen == del)
        onDeleteTrip(idx);
}

void MainWindow::onToggleTheme()
{
    const Theme next = (ThemeManager::instance().currentTheme() == Theme::Dark)
                           ? Theme::Light
                           : Theme::Dark;
    ThemeManager::instance().setTheme(next);
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(this);
    connect(&dlg, &SettingsDialog::dataCleared, this, &MainWindow::reloadTrips);
    // 延迟到设置对话框 exec 结束后再处理退出，避免嵌套模态
    connect(&dlg, &SettingsDialog::logoutRequested, this,
            [this]() { QTimer::singleShot(0, this, [this]() { onLogout(); }); });
    dlg.exec();
}

void MainWindow::onLogout()
{
    AuthController::instance().logout();
    hide();
    LoginDialog login(this);
    if (login.exec() != QDialog::Accepted) {
        QApplication::quit();
        return;
    }
    m_userLabel->setText(Session::instance().currentUser().username);
    reloadTrips();
    show();
}
