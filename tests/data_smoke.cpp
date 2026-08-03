// 数据层冒烟测试（无 GUI）：DB / PBKDF2 / 认证 / 行程 CRUD / 空数据 / 备份。
// 覆盖"空数据/异常数据不崩溃"自检项。返回 0 = 全部通过。
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include "app/databasemanager.h"
#include "app/pbkdf2.h"
#include "app/session.h"
#include "app/authcontroller.h"
#include "repo/triprepository.h"
#include "repo/userrepository.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (cond) qInfo().noquote() << "PASS:" << msg; \
    else { qWarning().noquote() << "FAIL:" << msg; ++failures; } \
} while (0)

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("TransitLogDataTest"));
    QCoreApplication::setOrganizationName(QStringLiteral("CDD-2333"));
    QStandardPaths::setTestModeEnabled(true);

    // 1. PBKDF2 往返
    const QByteArray h = PBKDF2::hashPassword(QStringLiteral("密码abc123"));
    CHECK(h.startsWith("pbkdf2$"), "PBKDF2 格式前缀");
    CHECK(PBKDF2::verifyPassword(QStringLiteral("密码abc123"), h), "PBKDF2 正确密码");
    CHECK(!PBKDF2::verifyPassword(QStringLiteral("wrong"), h), "PBKDF2 错误密码");

    // 2. 打开数据库
    CHECK(DatabaseManager::instance().open(), "打开数据库");

    AuthController& auth = AuthController::instance();
    TripRepository repo;

    // 3. 注册 + 重复注册
    QString err;
    CHECK(auth.registerUser(QStringLiteral("test_user"), QStringLiteral("pw123"),
                            QStringLiteral("测试"), err), "注册用户");
    CHECK(!auth.registerUser(QStringLiteral("test_user"), QStringLiteral("pw123"), QString(), err),
          "重复注册被拒绝");

    // 4. 登录
    CHECK(auth.login(QStringLiteral("test_user"), QStringLiteral("pw123"), err), "登录成功");
    CHECK(Session::instance().isLoggedIn(), "会话已登录");
    CHECK(!auth.login(QStringLiteral("test_user"), QStringLiteral("bad"), err), "错误密码拒绝");
    CHECK(!auth.login(QStringLiteral("nobody"), QStringLiteral("x"), err), "不存在用户拒绝");

    const QString uid = Session::instance().userId();
    CHECK(!uid.isEmpty(), "用户 id 非空");

    // 5. 字典种子
    CHECK(repo.transportModes().size() > 5, "交通方式字典已种子化");
    CHECK(repo.tripTags().size() >= 4, "标签字典已种子化");

    // 6. 新增行程（完整数据）
    Trip t1;
    t1.userId = uid;
    t1.modeCode = QStringLiteral("SUBWAY");
    t1.startTime = QDateTime::currentDateTimeUtc().addDays(-1);
    t1.endTime = t1.startTime.addSecs(1800);
    t1.startPlace = QStringLiteral("海淀黄庄");
    t1.endPlace = QStringLiteral("北京大学东门");
    t1.distanceM = 3200;
    t1.costFen = 400;
    t1.tagId = QStringLiteral("COMMUTE");
    t1.note = QStringLiteral("早高峰");
    CHECK(repo.saveTrip(t1), "保存行程(完整)");

    // 7. 进行中行程（end_time 空，不填里程费用）
    Trip t2;
    t2.userId = uid;
    t2.modeCode = QStringLiteral("DRIVE");
    t2.startTime = QDateTime::currentDateTimeUtc();
    t2.startPlace = QStringLiteral("未名湖");
    CHECK(repo.saveTrip(t2), "保存行程(进行中/空字段)");

    // 8. 只填时间的行程
    Trip t3;
    t3.userId = uid;
    t3.modeCode = QStringLiteral("WALK");
    t3.startTime = QDateTime::currentDateTimeUtc();
    CHECK(repo.saveTrip(t3), "保存行程(仅时间)");

    // 9. 查询
    const QList<Trip> all = repo.tripsForUser(uid);
    CHECK(all.size() == 3, QStringLiteral("查询到 3 条行程(实际 %1)").arg(all.size()));
    bool hasInProgress = false;
    for (const Trip& t : all) {
        if (t.isInProgress())
            hasInProgress = true;
        if (t.id.isEmpty())
            CHECK(false, "行程 id 非空");
    }
    CHECK(hasInProgress, "进行中行程正确返回(空 end_time)");

    // 10. 编辑
    Trip edit = all.first();
    edit.note = QStringLiteral("已修改");
    CHECK(repo.saveTrip(edit), "编辑行程");
    CHECK(repo.tripsForUser(uid).size() == 3, "编辑不产生新行");

    // 11. 软删除
    CHECK(repo.softDeleteTrip(edit.id), "软删除");
    CHECK(repo.tripsForUser(uid).size() == 2, "删除后剩 2 条");

    // 12. 统计（空/非空都不崩溃）
    const QDate today = QDate::currentDate();
    const auto stats = repo.dailyStats(uid, today.addDays(-6), today);
    CHECK(true, QStringLiteral("dailyStats 返回 %1 天").arg(stats.size()));
    const auto modes = repo.modeStats(uid);
    CHECK(true, QStringLiteral("modeStats 返回 %1 类").arg(modes.size()));

    // 13. 改密 + 新密码登录
    CHECK(auth.changePassword(QStringLiteral("pw123"), QStringLiteral("newpw"), err), "修改密码");
    CHECK(auth.login(QStringLiteral("test_user"), QStringLiteral("newpw"), err), "新密码登录成功");
    auth.logout();
    CHECK(!Session::instance().isLoggedIn(), "退出登录");

    // 14. 备份
    const QString backup = QDir::tempPath() + QStringLiteral("/transitlog_backup_test.db");
    QFile::remove(backup);
    QString berr;
    CHECK(DatabaseManager::instance().backupTo(backup, &berr), "VACUUM INTO 备份");
    CHECK(QFile::exists(backup), "备份文件存在");
    QFile::remove(backup);

    qInfo() << (failures == 0 ? "=== DATA_SMOKE_OK ===" : "=== FAILURES: " + QString::number(failures) + " ===");
    return failures == 0 ? 0 : 1;
}
