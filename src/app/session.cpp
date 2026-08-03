#include "session.h"

Session& Session::instance()
{
    static Session s;
    return s;
}

bool Session::isLoggedIn() const
{
    return m_loggedIn;
}

User Session::currentUser() const
{
    return m_user;
}

QString Session::userId() const
{
    return m_user.id;
}

void Session::start(const User& user)
{
    m_user = user;
    m_loggedIn = true;
}

void Session::end()
{
    m_user = User();
    m_loggedIn = false;
}
