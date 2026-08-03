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

    return migrate();
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
        seedDictionaries();
    }
    return true;
}

void DatabaseManager::seedDictionaries()
{
    // 交通方式字典
    {
        QSqlQuery count(m_db);
        count.exec(QStringLiteral("SELECT COUNT(*) FROM transport_mode"));
        bool hasRows = false;
        if (count.next())
            hasRows = count.value(0).toInt() > 0;

        if (!hasRows) {
            struct SeedMode {
                QString code, name, icon;
                double speed;
                int sort;
            };
            const SeedMode modes[] = {
                {QStringLiteral("WALK"),   QStringLiteral("步行"),     QStringLiteral("🚶"), 5.0,   1},
                {QStringLiteral("BIKE"),   QStringLiteral("骑行"),     QStringLiteral("🚲"), 15.0,  2},
                {QStringLiteral("BUS"),    QStringLiteral("公交"),     QStringLiteral("🚌"), 25.0,  3},
                {QStringLiteral("SUBWAY"), QStringLiteral("地铁"),     QStringLiteral("🚇"), 35.0,  4},
                {QStringLiteral("DRIVE"),  QStringLiteral("驾车"),     QStringLiteral("🚗"), 40.0,  5},
                {QStringLiteral("TAXI"),   QStringLiteral("出租车"),   QStringLiteral("🚕"), 40.0,  6},
                {QStringLiteral("RAIL"),   QStringLiteral("火车/高铁"), QStringLiteral("🚄"), 200.0, 7},
                {QStringLiteral("FLIGHT"), QStringLiteral("飞机"),     QStringLiteral("✈️"), 700.0, 8},
                {QStringLiteral("OTHER"),  QStringLiteral("其他"),     QStringLiteral("🚩"), 20.0,  9},
            };
            for (const SeedMode& m : modes) {
                QSqlQuery ins(m_db);
                ins.prepare(QStringLiteral(
                    "INSERT INTO transport_mode (code, name, icon, default_speed_kmh, sort_order)"
                    " VALUES (?,?,?,?,?)"));
                ins.addBindValue(m.code);
                ins.addBindValue(m.name);
                ins.addBindValue(m.icon);
                ins.addBindValue(m.speed);
                ins.addBindValue(m.sort);
                ins.exec();
            }
        }
    }

    // 行程标签字典
    {
        QSqlQuery count(m_db);
        count.exec(QStringLiteral("SELECT COUNT(*) FROM trip_tag"));
        bool hasRows = false;
        if (count.next())
            hasRows = count.value(0).toInt() > 0;

        if (!hasRows) {
            struct SeedTag {
                QString code, name, color;
                int sort;
            };
            const SeedTag tags[] = {
                {QStringLiteral("COMMUTE"), QStringLiteral("通勤"), QStringLiteral("#4A7C6F"), 1},
                {QStringLiteral("BUSINESS"), QStringLiteral("出差"), QStringLiteral("#2D6B9F"), 2},
                {QStringLiteral("TRAVEL"), QStringLiteral("旅游"), QStringLiteral("#B5742D"), 3},
                {QStringLiteral("OTHER"), QStringLiteral("其他"), QStringLiteral("#888888"), 4},
            };
            for (const SeedTag& t : tags) {
                QSqlQuery ins(m_db);
                ins.prepare(QStringLiteral(
                    "INSERT INTO trip_tag (code, name, color, sort_order) VALUES (?,?,?,?)"));
                ins.addBindValue(t.code);
                ins.addBindValue(t.name);
                ins.addBindValue(t.color);
                ins.addBindValue(t.sort);
                ins.exec();
            }
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
