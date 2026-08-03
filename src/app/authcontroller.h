#pragma once

#include <QString>

#include "repo/userrepository.h"

// 认证控制器：UI 层只调这里，SQL 在 UserRepository 内。
// 成功登录/注册会自动写入 Session。
class AuthController {
public:
    static AuthController& instance();

    bool login(const QString& username, const QString& password, QString& error);
    bool registerUser(const QString& username, const QString& password,
                      const QString& nickname, QString& error);
    bool changePassword(const QString& oldPassword, const QString& newPassword, QString& error);
    void logout();
    QString currentUsername() const;
    bool hasAnyUser() const;

private:
    AuthController() = default;

    UserRepository m_repo;
};
