// GUI 冒烟测试：构造主窗口、注入行程数据、渲染并抓图，验证不崩溃。
// 用法：QT_QPA_PLATFORM=offscreen ./TransitLogGuiSmoke
#include <QApplication>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>

#include "app/authcontroller.h"
#include "app/databasemanager.h"
#include "app/session.h"
#include "app/thememanager.h"
#include "repo/triprepository.h"
#include "ui/appdatetimeedit.h"
#include "ui/mainwindow.h"
#include "ui/tripeditdialog.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TransitLogGuiTest"));
    QApplication::setOrganizationName(QStringLiteral("CDD-2333"));
    QStandardPaths::setTestModeEnabled(true);

    if (!DatabaseManager::instance().open()) {
        qCritical() << "open db failed";
        return 1;
    }
    QString err;
    if (!AuthController::instance().registerUser(QStringLiteral("gui_test"),
                                                 QStringLiteral("pw"), QString(), err)) {
        qCritical() << "register failed:" << err;
        return 1;
    }
    const QString uid = Session::instance().userId();

    // 造数据：完整行程 + 进行中行程 + 仅时间行程 + 空备注
    TripRepository repo;
    {
        Trip t;
        t.userId = uid;
        t.modeCode = QStringLiteral("SUBWAY");
        t.startTime = QDateTime::currentDateTimeUtc().addDays(-1);
        t.endTime = t.startTime.addSecs(1800);
        t.startPlace = QStringLiteral("海淀黄庄");
        t.endPlace = QStringLiteral("北京大学东门");
        t.distanceM = 3200;
        t.costFen = 400;
        t.tagId = QStringLiteral("COMMUTE");
        t.vehicleNo = QStringLiteral("302路");
        t.vehicleModel = QStringLiteral("CR400AF");
        t.note = QStringLiteral("早高峰人少");
        if (!repo.saveTrip(t))
            qCritical() << "save t1 failed";
    }
    {
        Trip t;
        t.userId = uid;
        t.modeCode = QStringLiteral("DRIVE");
        t.startTime = QDateTime::currentDateTimeUtc();
        t.startPlace = QStringLiteral("未名湖");
        if (!repo.saveTrip(t))
            qCritical() << "save t2 failed";
    }
    {
        Trip t;
        t.userId = uid;
        t.modeCode = QStringLiteral("WALK");
        t.startTime = QDateTime::currentDateTimeUtc().addDays(-2);
        if (!repo.saveTrip(t))
            qCritical() << "save t3 failed";
    }

    // 与 main.cpp 一致：注册 Tabler 图标字体（交通方式图标渲染）
    QFontDatabase::addApplicationFont(QStringLiteral(":/resources/fonts/tabler-icons.ttf"));

    app.setStyleSheet(ThemeManager::instance().buildQSS(ThemeManager::instance().currentTheme()));

    int failures = 0;

    // QComboBox 下拉箭头渲染断言：浅色箭头 #6B7280 / 深色 #9AA0A8（text-secondary token）
    auto comboArrowPixels = [&](Theme t, const QColor& target) {
        ThemeManager::instance().setTheme(t);
        app.setStyleSheet(ThemeManager::instance().buildQSS(t));
        QComboBox cb;
        cb.addItem(QStringLiteral("选项"));
        cb.resize(220, 40);
        cb.show();
        QApplication::processEvents();
        const QPixmap pm = cb.grab();
        cb.hide();
        if (pm.isNull())
            return 0;
        int cnt = 0;
        for (int y = 0; y < pm.height(); ++y)
            for (int x = pm.width() - 80; x < pm.width(); ++x) {
                const QColor c = pm.toImage().pixelColor(x, y);
                if (std::abs(c.red() - target.red()) <= 25 && std::abs(c.green() - target.green()) <= 25
                    && std::abs(c.blue() - target.blue()) <= 25)
                    ++cnt;
            }
        return cnt;
    };
    const int lightPx = comboArrowPixels(Theme::Light, QColor(0x6B, 0x72, 0x80));
    const int darkPx = comboArrowPixels(Theme::Dark, QColor(0x9A, 0xA0, 0xA8));
    qInfo() << "combo arrow pixels light=" << lightPx << "dark=" << darkPx;
    if (lightPx < 10 || darkPx < 10) {
        qCritical() << "QComboBox 下拉箭头未渲染";
        ++failures;
    }

    // QDateTimeEdit（时间选择器）：无箭头/按钮，点击输入框任意位置应弹出日历
    struct ShowFilter : QObject {
        int showCount = 0;
        bool eventFilter(QObject* o, QEvent* e) override
        {
            if (e->type() == QEvent::Show)
                ++showCount;
            return QObject::eventFilter(o, e);
        }
    };
    auto dateTimeClickOpensCalendar = [&]() {
        ThemeManager::instance().setTheme(Theme::Light);
        app.setStyleSheet(ThemeManager::instance().buildQSS(Theme::Light));
        AppDateTimeEdit de(QDateTime::currentDateTime());
        de.setCalendarPopup(true);
        de.resize(240, 40);
        de.show();
        QApplication::processEvents();
        ShowFilter f;
        de.calendarWidget()->installEventFilter(&f);
        // 点击输入框中间（非右侧按钮区）
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(120, 20), Qt::LeftButton,
                          Qt::LeftButton, Qt::NoModifier);
        QMouseEvent rel(QEvent::MouseButtonRelease, QPointF(120, 20), Qt::LeftButton,
                        Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&de, &press);
        QApplication::sendEvent(&de, &rel);
        QApplication::processEvents();
        de.hide();
        return f.showCount;
    };
    const int calShows = dateTimeClickOpensCalendar();
    qInfo() << "QDateTimeEdit 点击输入框弹出日历 Show 次数=" << calShows;
    if (calShows < 1) {
        qCritical() << "QDateTimeEdit 点击输入框未弹出日历";
        ++failures;
    }

    // 聚焦边框一致性：QLineEdit(起点/终点) 与 AppDateTimeEdit(开始/结束时间) 聚焦后
    // 顶边都应为同一 primary 色（共享 :focus 规则，不因控件类型而异）
    auto focusedPrimaryPixels = [&](QWidget* w) {
        w->setFocus(Qt::OtherFocusReason);
        w->resize(200, 40);
        w->show();
        QApplication::processEvents();
        const QImage img = w->grab().toImage();
        w->hide();
        int cnt = 0;
        for (int y = 0; y < 3; ++y)
            for (int x = 4; x < img.width() - 4; ++x) {
                const QColor c = img.pixelColor(x, y);
                if (std::abs(c.red() - 0x4A) <= 30 && std::abs(c.green() - 0x7C) <= 30
                    && std::abs(c.blue() - 0x6F) <= 30)
                    ++cnt;
            }
        return cnt;
    };
    ThemeManager::instance().setTheme(Theme::Light);
    app.setStyleSheet(ThemeManager::instance().buildQSS(Theme::Light));
    QLineEdit le;
    AppDateTimeEdit de(QDateTime::currentDateTime());
    const int leFocus = focusedPrimaryPixels(&le);
    const int deFocus = focusedPrimaryPixels(&de);
    qInfo() << "focus primary-border pixels: QLineEdit=" << leFocus << "AppDateTimeEdit=" << deFocus;
    if (leFocus < 20 || deFocus < 20) {
        qCritical() << "输入框聚焦边框不一致";
        ++failures;
    }

    ThemeManager::instance().setTheme(Theme::Light);
    app.setStyleSheet(ThemeManager::instance().buildQSS(Theme::Light));

    MainWindow w;
    w.resize(720, 560);
    w.show();

    // 截图目录（可选）：设置 QT_SCREENSHOT_DIR 时保存浅/深主题截图供人工核对
    const QString shotDir = qEnvironmentVariable("QT_SCREENSHOT_DIR");
    if (!shotDir.isEmpty())
        QDir().mkpath(shotDir);

    // 异步渲染后抓图验证（分步延时，确保各控件完成绘制）
    auto findNav = [&](const QString& text) -> QPushButton* {
        const auto btns = w.findChildren<QPushButton*>();
        for (QPushButton* b : btns)
            if (b->text().contains(text))
                return b;
        return nullptr;
    };
    auto navStats = [&]() { return findNav(QStringLiteral("统计")); };
    auto saveShot = [&](const QString& name, QWidget* target) {
        const QPixmap pm = target->grab();
        if (pm.isNull() || pm.width() == 0) {
            qCritical() << "grab" << name << "failed";
            ++failures;
        } else if (!shotDir.isEmpty()) {
            pm.save(shotDir + '/' + name);
        }
    };

    TripEditDialog* dlg = nullptr;
    QTimer::singleShot(200, [&]() {
        saveShot(QStringLiteral("trip-light.png"), &w);
        dlg = new TripEditDialog(repo.transportModes(), repo.tripTags(), Trip());
        dlg->resize(460, 560);
        dlg->show();
    });
    QTimer::singleShot(380, [&]() {
        saveShot(QStringLiteral("edit-light.png"), dlg);
        // 两步向导：点"下一步"进入第 2 页（车次/车型），截图验证
        if (dlg) {
            const auto btns = dlg->findChildren<QPushButton*>();
            for (QPushButton* b : btns) {
                if (b->text() == QStringLiteral("下一步")) {
                    b->click();
                    QApplication::processEvents();
                    const QPixmap p2 = dlg->grab();
                    if (!p2.isNull() && !shotDir.isEmpty())
                        p2.save(shotDir + QStringLiteral("/edit-step2-light.png"));
                    break;
                }
            }
        }
        dlg->close();
        dlg->deleteLater();
        dlg = nullptr;
        if (QPushButton* b = navStats())
            b->click();
    });
    QTimer::singleShot(560, [&]() {
        saveShot(QStringLiteral("stats-light.png"), &w);
        if (QPushButton* b = findNav(QStringLiteral("图鉴")))
            b->click();
    });
    QTimer::singleShot(640, [&]() {
        saveShot(QStringLiteral("catalog-light.png"), &w);
        ThemeManager::instance().setTheme(Theme::Dark);
        if (QPushButton* b = findNav(QStringLiteral("行程")))
            b->click();
    });
    QTimer::singleShot(760, [&]() {
        saveShot(QStringLiteral("trip-dark.png"), &w);
        if (QPushButton* b = findNav(QStringLiteral("图鉴")))
            b->click();
    });
    QTimer::singleShot(840, [&]() {
        saveShot(QStringLiteral("catalog-dark.png"), &w);
        // 深色主题下再打开编辑对话框（含 起点/终点/开始时间/结束时间 四个输入框）
        dlg = new TripEditDialog(repo.transportModes(), repo.tripTags(), Trip());
        dlg->resize(460, 560);
        dlg->show();
    });
    QTimer::singleShot(920, [&]() {
        saveShot(QStringLiteral("edit-dark.png"), dlg);
        dlg->close();
        dlg->deleteLater();
        dlg = nullptr;
        // 时间选择器单独渲染（浅/深）
        if (!shotDir.isEmpty()) {
            ThemeManager::instance().setTheme(Theme::Light);
            app.setStyleSheet(ThemeManager::instance().buildQSS(Theme::Light));
            AppDateTimeEdit de(QDateTime::currentDateTime());
            de.setCalendarPopup(true);
            de.resize(260, 40);
            de.show();
            QApplication::processEvents();
            de.grab().save(shotDir + QStringLiteral("/datetime-light.png"));
            ThemeManager::instance().setTheme(Theme::Dark);
            app.setStyleSheet(ThemeManager::instance().buildQSS(Theme::Dark));
            de.grab().save(shotDir + QStringLiteral("/datetime-dark.png"));
            de.hide();
        }
        qInfo() << (failures == 0 ? "GUI_SMOKE_OK" : "GUI_SMOKE_FAIL");
        app.exit(failures == 0 ? 0 : 1);
    });

    return app.exec();
}
