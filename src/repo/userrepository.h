#pragma once

#include "model/entities.h"

// 用户相关的全部 SQL 集中在这里，UI 层禁止直接写 SQL。
class UserRepository {
public:
    struct AuthResult {
        bool ok = false;
        QString error;
        User user;
    };

    AuthResult registerUser(const QString& username, const QString& password,
                            const QString& nickname);
    AuthResult login(const QString& username, const QString& password);
    bool changePassword(const QString& userId, const QString& oldPassword,
                        const QString& newPassword, QString& error);
    bool hasAnyUser() const;
};
