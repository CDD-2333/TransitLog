#include "authcontroller.h"

#include "session.h"

AuthController& AuthController::instance()
{
    static AuthController c;
    return c;
}

bool AuthController::login(const QString& username, const QString& password, QString& error)
{
    const UserRepository::AuthResult res = m_repo.login(username, password);
    if (!res.ok) {
        error = res.error;
        return false;
    }
    Session::instance().start(res.user);
    return true;
}

bool AuthController::registerUser(const QString& username, const QString& password,
                                  const QString& nickname, QString& error)
{
    const UserRepository::AuthResult res = m_repo.registerUser(username, password, nickname);
    if (!res.ok) {
        error = res.error;
        return false;
    }
    Session::instance().start(res.user); // 注册即登录
    return true;
}

bool AuthController::changePassword(const QString& oldPassword, const QString& newPassword,
                                    QString& error)
{
    const QString uid = Session::instance().userId();
    return m_repo.changePassword(uid, oldPassword, newPassword, error);
}

void AuthController::logout()
{
    Session::instance().end();
}

QString AuthController::currentUsername() const
{
    return Session::instance().currentUser().username;
}

bool AuthController::hasAnyUser() const
{
    return m_repo.hasAnyUser();
}
