// GUI 冒烟测试：构造主窗口、注入行程数据、渲染并抓图，验证不崩溃。
// 用法：QT_QPA_PLATFORM=offscreen ./TransitLogGuiSmoke
#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QTimer>

#include "app/authcontroller.h"
#include "app/databasemanager.h"
#include "app/session.h"
#include "app/thememanager.h"
#include "repo/triprepository.h"
#include "ui/mainwindow.h"

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

    app.setStyleSheet(ThemeManager::instance().buildQSS(ThemeManager::instance().currentTheme()));

    MainWindow w;
    w.resize(720, 560);
    w.show();

    // 异步渲染后抓图验证
    int failures = 0;
    QTimer::singleShot(200, [&]() {
        const QPixmap trips = w.grab();
        if (trips.isNull() || trips.width() == 0) {
            qCritical() << "grab trips page failed";
            ++failures;
        }
        // 切到统计页再抓
        QTimer::singleShot(50, [&]() {
            const QPixmap stats = w.grab();
            if (stats.isNull() || stats.width() == 0) {
                qCritical() << "grab stats page failed";
                ++failures;
            }
            qInfo() << (failures == 0 ? "GUI_SMOKE_OK" : "GUI_SMOKE_FAIL");
            app.exit(failures == 0 ? 0 : 1);
        });
    });

    return app.exec();
}
