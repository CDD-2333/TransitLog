#pragma once

#include <QByteArray>
#include <QString>

// PBKDF2-HMAC-SHA256 口令派生（本地登录密码保护）。
// 存储格式：pbkdf2$迭代次数$盐hex$派生密钥hex
namespace PBKDF2 {

QByteArray hashPassword(const QString& password);
bool verifyPassword(const QString& password, const QByteArray& stored);

} // namespace PBKDF2
