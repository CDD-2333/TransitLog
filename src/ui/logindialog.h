#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QTabWidget;

// 本地登录 / 注册。成功后 Session 已写入，直接 accept()。
// 认证逻辑走 AuthController（其内部调用 UserRepository），不直接碰 SQL。
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

private slots:
    void onLogin();
    void onRegister();
    void onTabChanged(int index);

private:
    void buildUI();
    void showError(const QString& msg);

    QTabWidget* m_tabs = nullptr;
    QLineEdit* m_loginUser = nullptr;
    QLineEdit* m_loginPass = nullptr;
    QLineEdit* m_regUser = nullptr;
    QLineEdit* m_regNick = nullptr;
    QLineEdit* m_regPass = nullptr;
    QLineEdit* m_regPass2 = nullptr;
    QLabel* m_errorLabel = nullptr;
};
