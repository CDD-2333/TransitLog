#include "settingsdialog.h"

#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "app/authcontroller.h"
#include "app/databasemanager.h"
#include "app/session.h"
#include "app/thememanager.h"
#include "repo/triprepository.h"

namespace {

// 修改密码的小对话框
class ChangePasswordDialog : public QDialog {
public:
    explicit ChangePasswordDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("修改密码"));
        setMinimumWidth(320);
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(20, 16, 20, 16);

        auto* form = new QFormLayout;
        m_old = new QLineEdit(this);
        m_old->setEchoMode(QLineEdit::Password);
        m_new = new QLineEdit(this);
        m_new->setEchoMode(QLineEdit::Password);
        m_new2 = new QLineEdit(this);
        m_new2->setEchoMode(QLineEdit::Password);
        form->addRow(QStringLiteral("原密码"), m_old);
        form->addRow(QStringLiteral("新密码"), m_new);
        form->addRow(QStringLiteral("确认新密码"), m_new2);
        root->addLayout(form);

        m_error = new QLabel(this);
        m_error->setStyleSheet(QStringLiteral("color: #E24B4A;"));
        m_error->setWordWrap(true);
        m_error->hide();
        root->addWidget(m_error);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        root->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (m_new->text() != m_new2->text()) {
                m_error->setText(QStringLiteral("两次输入的新密码不一致"));
                m_error->show();
                return;
            }
            QString err;
            if (!AuthController::instance().changePassword(m_old->text(), m_new->text(), err)) {
                m_error->setText(err);
                m_error->show();
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

private:
    QLineEdit* m_old = nullptr;
    QLineEdit* m_new = nullptr;
    QLineEdit* m_new2 = nullptr;
    QLabel* m_error = nullptr;
};

QFrame* sectionCard(QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("card"));
    return card;
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("设置"));
    setMinimumWidth(420);
    buildUI();
}

void SettingsDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // ---- 外观 ----
    auto* themeCard = sectionCard(this);
    auto* themeLayout = new QVBoxLayout(themeCard);
    themeLayout->setContentsMargins(16, 12, 16, 12);
    auto* themeTitle = new QLabel(QStringLiteral("外观"), themeCard);
    themeTitle->setObjectName(QStringLiteral("hintLabel"));
    themeLayout->addWidget(themeTitle);
    m_themeCombo = new QComboBox(themeCard);
    m_themeCombo->addItem(QStringLiteral("浅色（纸感）"), QStringLiteral("light"));
    m_themeCombo->addItem(QStringLiteral("深色"), QStringLiteral("dark"));
    const Theme cur = ThemeManager::instance().currentTheme();
    m_themeCombo->setCurrentIndex(cur == Theme::Dark ? 1 : 0);
    themeLayout->addWidget(m_themeCombo);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeChanged);
    root->addWidget(themeCard);

    // ---- 账户 ----
    auto* accountCard = sectionCard(this);
    auto* accountLayout = new QVBoxLayout(accountCard);
    accountLayout->setContentsMargins(16, 12, 16, 12);
    auto* accountTitle = new QLabel(QStringLiteral("账户"), accountCard);
    accountTitle->setObjectName(QStringLiteral("hintLabel"));
    accountLayout->addWidget(accountTitle);
    m_userLabel = new QLabel(Session::instance().currentUser().username, accountCard);
    accountLayout->addWidget(m_userLabel);
    auto* accountBtns = new QHBoxLayout;
    auto* changePwdBtn = new QPushButton(QStringLiteral("修改密码"), accountCard);
    changePwdBtn->setObjectName(QStringLiteral("iconButton"));
    auto* logoutBtn = new QPushButton(QStringLiteral("退出登录"), accountCard);
    logoutBtn->setObjectName(QStringLiteral("dangerButton"));
    accountBtns->addWidget(changePwdBtn);
    accountBtns->addWidget(logoutBtn);
    accountBtns->addStretch();
    accountLayout->addLayout(accountBtns);
    connect(changePwdBtn, &QPushButton::clicked, this, &SettingsDialog::onChangePassword);
    connect(logoutBtn, &QPushButton::clicked, this, &SettingsDialog::onLogout);
    root->addWidget(accountCard);

    // ---- 数据管理 ----
    auto* dataCard = sectionCard(this);
    auto* dataLayout = new QVBoxLayout(dataCard);
    dataLayout->setContentsMargins(16, 12, 16, 12);
    auto* dataTitle = new QLabel(QStringLiteral("数据管理"), dataCard);
    dataTitle->setObjectName(QStringLiteral("hintLabel"));
    dataLayout->addWidget(dataTitle);
    m_dbPathLabel = new QLabel(DatabaseManager::instance().dbPath(), dataCard);
    m_dbPathLabel->setObjectName(QStringLiteral("dbPathLabel"));
    m_dbPathLabel->setWordWrap(true);
    dataLayout->addWidget(m_dbPathLabel);
    auto* dataBtns = new QHBoxLayout;
    auto* backupBtn = new QPushButton(QStringLiteral("导出备份 .db"), dataCard);
    backupBtn->setObjectName(QStringLiteral("iconButton"));
    auto* clearBtn = new QPushButton(QStringLiteral("清空本地数据"), dataCard);
    clearBtn->setObjectName(QStringLiteral("dangerButton"));
    dataBtns->addWidget(backupBtn);
    dataBtns->addWidget(clearBtn);
    dataBtns->addStretch();
    dataLayout->addLayout(dataBtns);
    connect(backupBtn, &QPushButton::clicked, this, &SettingsDialog::onExportBackup);
    connect(clearBtn, &QPushButton::clicked, this, &SettingsDialog::onClearData);
    root->addWidget(dataCard);

    root->addStretch();

    auto* closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setObjectName(QStringLiteral("primaryButton"));
    root->addWidget(closeBtn, 0, Qt::AlignRight);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void SettingsDialog::onThemeChanged(int index)
{
    ThemeManager::instance().setTheme(index == 1 ? Theme::Dark : Theme::Light);
}

void SettingsDialog::onChangePassword()
{
    ChangePasswordDialog dlg(this);
    dlg.exec();
}

void SettingsDialog::onExportBackup()
{
    const QString defaultName = QStringLiteral("TransitLog_backup_%1.db")
                                    .arg(QDate::currentDate().toString(QStringLiteral("yyyyMMdd")));
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出备份"),
                                                      defaultName, QStringLiteral("数据库 (*.db)"));
    if (path.isEmpty())
        return;
    QString err;
    if (DatabaseManager::instance().backupTo(path, &err)) {
        QMessageBox::information(this, QStringLiteral("备份完成"),
                                 QStringLiteral("已导出到：\n%1").arg(path));
    } else {
        QMessageBox::warning(this, QStringLiteral("备份失败"), err);
    }
}

void SettingsDialog::onClearData()
{
    const auto ret = QMessageBox::question(
        this, QStringLiteral("清空本地数据"),
        QStringLiteral("将删除当前账号的所有行程记录，且不可恢复。确定继续吗？"));
    if (ret != QMessageBox::Yes)
        return;

    TripRepository repo;
    const bool ok = repo.hardDeleteAllForUser(Session::instance().userId());
    if (ok) {
        QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("本地行程数据已清空"));
        emit dataCleared();
    } else {
        QMessageBox::warning(this, QStringLiteral("失败"), QStringLiteral("清空数据失败，请重试"));
    }
}

void SettingsDialog::onLogout()
{
    const auto ret = QMessageBox::question(this, QStringLiteral("退出登录"),
                                           QStringLiteral("确认退出当前账号？"));
    if (ret != QMessageBox::Yes)
        return;
    accept();
    emit logoutRequested();
}
