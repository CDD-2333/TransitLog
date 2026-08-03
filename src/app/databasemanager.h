#pragma once

#include <QSqlDatabase>
#include <QString>

// SQLite 连接管理 + schema 迁移 + 字典种子数据。
// 所有 Repository 通过 DatabaseManager::instance().db() 取连接，SQL 一律不进 UI 层。
class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool open();
    void close();
    QSqlDatabase db() const;
    QString dbPath() const;
    QString lastError() const;

    // 用 VACUUM INTO 生成一致的备份副本（拷贝正在使用的 SQLite 文件不安全）
    bool backupTo(const QString& targetPath, QString* error = nullptr);

private:
    DatabaseManager() = default;

    bool migrate();
    void syncDictionaries();

    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_lastError;
};
