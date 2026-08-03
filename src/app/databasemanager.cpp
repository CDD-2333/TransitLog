#include "databasemanager.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager mgr;
    return mgr;
}

QSqlDatabase DatabaseManager::db() const
{
    return m_db;
}

QString DatabaseManager::dbPath() const
{
    return m_dbPath;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

bool DatabaseManager::open()
{
    if (m_db.isOpen())
        return true;

    m_dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
             + QStringLiteral("/TransitLog.db");
    const QDir dir = QFileInfo(m_dbPath).dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        m_lastError = QStringLiteral("无法创建数据目录: %1").arg(dir.absolutePath());
        return false;
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("transitlog"));
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    pragma.exec(QStringLiteral("PRAGMA encoding = 'UTF-8'"));

    if (!migrate())
        return false;
    syncDictionaries(); // 每次启动同步字典（幂等，兼容已有数据库）
    return true;
}

void DatabaseManager::close()
{
    const QString connName = m_db.connectionName();
    if (m_db.isOpen())
        m_db.close();
    if (!connName.isEmpty())
        QSqlDatabase::removeDatabase(connName);
    m_db = QSqlDatabase();
}

bool DatabaseManager::migrate()
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("PRAGMA user_version"))) {
        m_lastError = q.lastError().text();
        return false;
    }
    int version = 0;
    if (q.next())
        version = q.value(0).toInt();

    if (version < 1) {
        const QStringList stmts = {
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS user ("
                " id TEXT PRIMARY KEY,"
                " username TEXT NOT NULL UNIQUE,"
                " password_hash TEXT NOT NULL,"
                " nickname TEXT NOT NULL DEFAULT '',"
                " avatar_path TEXT,"
                " created_at INTEGER NOT NULL,"
                " updated_at INTEGER NOT NULL)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS transport_mode ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " code TEXT NOT NULL UNIQUE,"
                " name TEXT NOT NULL,"
                " icon TEXT NOT NULL DEFAULT '',"
                " default_speed_kmh REAL,"
                " sort_order INTEGER NOT NULL DEFAULT 0,"
                " is_active INTEGER NOT NULL DEFAULT 1)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS trip_tag ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " code TEXT NOT NULL UNIQUE,"
                " name TEXT NOT NULL,"
                " color TEXT NOT NULL DEFAULT '#888888',"
                " sort_order INTEGER NOT NULL DEFAULT 0,"
                " is_active INTEGER NOT NULL DEFAULT 1)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS trip_record ("
                " id TEXT PRIMARY KEY,"
                " user_id TEXT NOT NULL REFERENCES user(id),"
                " mode_code TEXT NOT NULL REFERENCES transport_mode(code),"
                " start_time INTEGER NOT NULL,"
                " end_time INTEGER,"
                " start_place TEXT NOT NULL DEFAULT '',"
                " end_place TEXT NOT NULL DEFAULT '',"
                " start_lat REAL,"
                " start_lng REAL,"
                " end_lat REAL,"
                " end_lng REAL,"
                " distance_m INTEGER,"
                " cost_fen INTEGER,"
                " tag_id TEXT REFERENCES trip_tag(code),"
                " note TEXT NOT NULL DEFAULT '',"
                " is_deleted INTEGER NOT NULL DEFAULT 0,"
                " created_at INTEGER NOT NULL,"
                " updated_at INTEGER NOT NULL)"),
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_trip_user_time ON trip_record(user_id, start_time DESC)"),
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_trip_user_del ON trip_record(user_id, is_deleted)"),
        };
        for (const QString& sql : stmts) {
            if (!q.exec(sql)) {
                m_lastError = q.lastError().text();
                return false;
            }
        }
        if (!q.exec(QStringLiteral("PRAGMA user_version = 1"))) {
            m_lastError = q.lastError().text();
            return false;
        }
    }

    // v2：车次（302路/K262次/航班号）与车型（CR400AF/DF4D）两个可空字段
    if (version < 2) {
        const QStringList stmts = {
            QStringLiteral("ALTER TABLE trip_record ADD COLUMN vehicle_no TEXT"),
            QStringLiteral("ALTER TABLE trip_record ADD COLUMN vehicle_model TEXT"),
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_trip_user_vehicle_no ON trip_record(user_id, vehicle_no)"),
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_trip_user_vehicle_mdl ON trip_record(user_id, vehicle_model)"),
        };
        for (const QString& sql : stmts) {
            if (!q.exec(sql)) {
                m_lastError = q.lastError().text();
                return false;
            }
        }
        if (!q.exec(QStringLiteral("PRAGMA user_version = 2"))) {
            m_lastError = q.lastError().text();
            return false;
        }
    }
    return true;
}

// Tabler 图标字体的字形码（见 resources/fonts/tabler-icons.ttf）
namespace {
constexpr QChar kGlyphBus(0xEBE4);
constexpr QChar kGlyphTrain(0xED96);
constexpr QChar kGlyphPlane(0xEB6F);
constexpr QChar kGlyphFlag(0xEAA6);
} // namespace

// 字典同步：幂等，每次启动执行。新装库插入种子；已有库更新名称/图标、软删停用项。
void DatabaseManager::syncDictionaries()
{
    // 交通方式字典（icon 存 Tabler 字形字符，替代 emoji；WALK/BIKE/DRIVE/TAXI 停用）
    {
        struct SeedMode {
            QString code, name;
            QChar icon;
            double speed;
            int sort;
            bool active;
        };
        const SeedMode modes[] = {
            {QStringLiteral("BUS"),    QStringLiteral("公交"),       kGlyphBus,   25.0,  3, true},
            {QStringLiteral("SUBWAY"), QStringLiteral("地铁"),       kGlyphTrain, 35.0,  4, true},
            {QStringLiteral("RAIL"),   QStringLiteral("火车"),       kGlyphTrain, 200.0, 7, true},
            {QStringLiteral("FLIGHT"), QStringLiteral("飞机"),       kGlyphPlane, 700.0, 8, true},
            {QStringLiteral("OTHER"),  QStringLiteral("其他"),       kGlyphFlag,  20.0,  9, true},
            {QStringLiteral("WALK"),   QStringLiteral("步行"),       kGlyphFlag,  5.0,   1, false},
            {QStringLiteral("BIKE"),   QStringLiteral("骑行"),       kGlyphFlag,  15.0,  2, false},
            {QStringLiteral("DRIVE"),  QStringLiteral("驾车"),       kGlyphFlag,  40.0,  5, false},
            {QStringLiteral("TAXI"),   QStringLiteral("出租车"),     kGlyphFlag,  40.0,  6, false},
        };
        for (const SeedMode& m : modes) {
            QSqlQuery ins(m_db);
            ins.prepare(QStringLiteral(
                "INSERT OR IGNORE INTO transport_mode (code, name, icon, default_speed_kmh, sort_order, is_active)"
                " VALUES (?,?,?,?,?,?)"));
            ins.addBindValue(m.code);
            ins.addBindValue(m.name);
            ins.addBindValue(QString(m.icon));
            ins.addBindValue(m.speed);
            ins.addBindValue(m.sort);
            ins.addBindValue(m.active ? 1 : 0);
            ins.exec();

            QSqlQuery up(m_db);
            up.prepare(QStringLiteral(
                "UPDATE transport_mode SET name=?, icon=?, default_speed_kmh=?, sort_order=?, is_active=? WHERE code=?"));
            up.addBindValue(m.name);
            up.addBindValue(QString(m.icon));
            up.addBindValue(m.speed);
            up.addBindValue(m.sort);
            up.addBindValue(m.active ? 1 : 0);
            up.addBindValue(m.code);
            up.exec();
        }
    }

    // 行程标签字典（OTHER 显示名改为「运转」）
    {
        struct SeedTag {
            QString code, name, color;
            int sort;
        };
        const SeedTag tags[] = {
            {QStringLiteral("COMMUTE"),  QStringLiteral("通勤"), QStringLiteral("#4A7C6F"), 1},
            {QStringLiteral("BUSINESS"), QStringLiteral("出差"), QStringLiteral("#2D6B9F"), 2},
            {QStringLiteral("TRAVEL"),   QStringLiteral("旅游"), QStringLiteral("#B5742D"), 3},
            {QStringLiteral("OTHER"),    QStringLiteral("运转"), QStringLiteral("#888888"), 4},
        };
        for (const SeedTag& t : tags) {
            QSqlQuery ins(m_db);
            ins.prepare(QStringLiteral(
                "INSERT OR IGNORE INTO trip_tag (code, name, color, sort_order) VALUES (?,?,?,?)"));
            ins.addBindValue(t.code);
            ins.addBindValue(t.name);
            ins.addBindValue(t.color);
            ins.addBindValue(t.sort);
            ins.exec();

            QSqlQuery up(m_db);
            up.prepare(QStringLiteral(
                "UPDATE trip_tag SET name=?, color=?, sort_order=? WHERE code=?"));
            up.addBindValue(t.name);
            up.addBindValue(t.color);
            up.addBindValue(t.sort);
            up.addBindValue(t.code);
            up.exec();
        }
    }
}

bool DatabaseManager::backupTo(const QString& targetPath, QString* error)
{
    if (!m_db.isOpen()) {
        if (error)
            *error = QStringLiteral("数据库未打开");
        return false;
    }
    QString escaped = targetPath;
    escaped.replace(QStringLiteral("'"), QStringLiteral("''"));
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("VACUUM INTO '") + escaped + QStringLiteral("'"))) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return true;
}
