#pragma once

#include "model/entities.h"

// 当前登录会话（本地登录）。全应用通过它拿到当前用户，UI 不直接持有 SQL。
class Session {
public:
    static Session& instance();

    bool isLoggedIn() const;
    User currentUser() const;
    QString userId() const;

    void start(const User& user);
    void end();

private:
    Session() = default;

    User m_user;
    bool m_loggedIn = false;
};
