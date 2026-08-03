#include "userrepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include "app/databasemanager.h"
#include "app/pbkdf2.h"

namespace {

QDateTime nowUtc()
{
    return QDateTime::currentDateTimeUtc();
}

} // namespace

UserRepository::AuthResult UserRepository::registerUser(const QString& username,
                                                        const QString& password,
                                                        const QString& nickname)
{
    AuthResult result;
    const QString name = username.trimmed();
    if (name.isEmpty()) {
        result.error = QStringLiteral("用户名不能为空");
        return result;
    }
    if (password.isEmpty()) {
        result.error = QStringLiteral("密码不能为空");
        return result;
    }

    QSqlDatabase db = DatabaseManager::instance().db();
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral("SELECT 1 FROM user WHERE username = ?"));
        q.addBindValue(name);
        if (!q.exec()) {
            result.error = QStringLiteral("数据库错误: %1").arg(q.lastError().text());
            return result;
        }
        if (q.next()) {
            result.error = QStringLiteral("用户名已被注册");
            return result;
        }
    }

    User u;
    u.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    u.username = name;
    u.nickname = nickname.trimmed();
    u.createdAt = nowUtc();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO user (id, username, password_hash, nickname, created_at, updated_at)"
        " VALUES (?,?,?,?,?,?)"));
    q.addBindValue(u.id);
    q.addBindValue(u.username);
    q.addBindValue(QString::fromUtf8(PBKDF2::hashPassword(password)));
    q.addBindValue(u.nickname.isNull() ? QStringLiteral("") : u.nickname);
    q.addBindValue(u.createdAt.toMSecsSinceEpoch());
    q.addBindValue(u.createdAt.toMSecsSinceEpoch());

    if (!q.exec()) {
        result.error = QStringLiteral("注册失败: %1").arg(q.lastError().text());
        return result;
    }
    result.ok = true;
    result.user = u;
    return result;
}

UserRepository::AuthResult UserRepository::login(const QString& username, const QString& password)
{
    AuthResult result;
    const QString name = username.trimmed();
    if (name.isEmpty() || password.isEmpty()) {
        result.error = QStringLiteral("请输入用户名和密码");
        return result;
    }

    QSqlDatabase db = DatabaseManager::instance().db();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, username, nickname, password_hash, created_at FROM user WHERE username = ?"));
    q.addBindValue(name);
    if (!q.exec()) {
        result.error = QStringLiteral("数据库错误: %1").arg(q.lastError().text());
        return result;
    }
    if (!q.next()) {
        result.error = QStringLiteral("用户不存在");
        return result;
    }

    const QByteArray storedHash = q.value(3).toByteArray();
    if (!PBKDF2::verifyPassword(password, storedHash)) {
        result.error = QStringLiteral("密码错误");
        return result;
    }

    result.ok = true;
    result.user.id = q.value(0).toString();
    result.user.username = q.value(1).toString();
    result.user.nickname = q.value(2).toString();
    result.user.createdAt = QDateTime::fromMSecsSinceEpoch(q.value(4).toLongLong(), Qt::UTC);
    return result;
}

bool UserRepository::changePassword(const QString& userId, const QString& oldPassword,
                                    const QString& newPassword, QString& error)
{
    if (newPassword.isEmpty()) {
        error = QStringLiteral("新密码不能为空");
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance().db();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT password_hash FROM user WHERE id = ?"));
    q.addBindValue(userId);
    if (!q.exec() || !q.next()) {
        error = QStringLiteral("用户不存在");
        return false;
    }
    const QByteArray storedHash = q.value(0).toByteArray();
    if (!PBKDF2::verifyPassword(oldPassword, storedHash)) {
        error = QStringLiteral("原密码错误");
        return false;
    }

    QSqlQuery up(db);
    up.prepare(QStringLiteral(
        "UPDATE user SET password_hash = ?, updated_at = ? WHERE id = ?"));
    up.addBindValue(QString::fromUtf8(PBKDF2::hashPassword(newPassword)));
    up.addBindValue(nowUtc().toMSecsSinceEpoch());
    up.addBindValue(userId);
    if (!up.exec()) {
        error = QStringLiteral("更新失败: %1").arg(up.lastError().text());
        return false;
    }
    return true;
}

bool UserRepository::hasAnyUser() const
{
    QSqlQuery q(DatabaseManager::instance().db());
    if (!q.exec(QStringLiteral("SELECT 1 FROM user LIMIT 1")))
        return false;
    return q.next();
}
