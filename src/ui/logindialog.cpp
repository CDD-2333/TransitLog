#include "logindialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "app/authcontroller.h"

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("TransitLog · 本地登录"));
    setMinimumWidth(360);
    buildUI();

    // 首次运行（无任何用户）默认落在注册页
    if (!AuthController::instance().hasAnyUser())
        m_tabs->setCurrentIndex(1);
}

void LoginDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("TransitLog"), this);
    title->setObjectName(QStringLiteral("appTitle"));
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("行程记录 · 数据仅保存在本机，不会上传"), this);
    subtitle->setObjectName(QStringLiteral("hintLabel"));
    subtitle->setAlignment(Qt::AlignCenter);
    root->addWidget(subtitle);

    m_tabs = new QTabWidget(this);
    {
        // 登录页
        auto* loginPage = new QWidget(this);
        auto* loginForm = new QFormLayout(loginPage);
        loginForm->setSpacing(10);
        m_loginUser = new QLineEdit(loginPage);
        m_loginUser->setPlaceholderText(QStringLiteral("用户名"));
        m_loginPass = new QLineEdit(loginPage);
        m_loginPass->setEchoMode(QLineEdit::Password);
        m_loginPass->setPlaceholderText(QStringLiteral("密码"));
        loginForm->addRow(QStringLiteral("用户名"), m_loginUser);
        loginForm->addRow(QStringLiteral("密码"), m_loginPass);
        auto* loginBtn = new QPushButton(QStringLiteral("登录"), loginPage);
        loginBtn->setObjectName(QStringLiteral("primaryButton"));
        loginForm->addRow(QString(), loginBtn);
        connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
        connect(m_loginPass, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
        m_tabs->addTab(loginPage, QStringLiteral("登录"));
    }
    {
        // 注册页
        auto* regPage = new QWidget(this);
        auto* regForm = new QFormLayout(regPage);
        regForm->setSpacing(10);
        m_regUser = new QLineEdit(regPage);
        m_regUser->setPlaceholderText(QStringLiteral("用户名"));
        m_regPass = new QLineEdit(regPage);
        m_regPass->setEchoMode(QLineEdit::Password);
        m_regPass->setPlaceholderText(QStringLiteral("密码"));
        m_regPass2 = new QLineEdit(regPage);
        m_regPass2->setEchoMode(QLineEdit::Password);
        m_regPass2->setPlaceholderText(QStringLiteral("确认密码"));
        regForm->addRow(QStringLiteral("用户名"), m_regUser);
        regForm->addRow(QStringLiteral("密码"), m_regPass);
        regForm->addRow(QStringLiteral("确认密码"), m_regPass2);
        auto* regBtn = new QPushButton(QStringLiteral("注册并登录"), regPage);
        regBtn->setObjectName(QStringLiteral("primaryButton"));
        regForm->addRow(QString(), regBtn);
        connect(regBtn, &QPushButton::clicked, this, &LoginDialog::onRegister);
        connect(m_regPass2, &QLineEdit::returnPressed, this, &LoginDialog::onRegister);
        m_tabs->addTab(regPage, QStringLiteral("注册"));
    }
    root->addWidget(m_tabs);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("hintLabel"));
    m_errorLabel->setStyleSheet(QStringLiteral("color: #E24B4A;"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    connect(m_tabs, &QTabWidget::currentChanged, this, &LoginDialog::onTabChanged);
}

void LoginDialog::showError(const QString& msg)
{
    m_errorLabel->setText(msg);
    m_errorLabel->show();
}

void LoginDialog::onTabChanged(int)
{
    m_errorLabel->hide();
}

void LoginDialog::onLogin()
{
    QString error;
    if (!AuthController::instance().login(m_loginUser->text(), m_loginPass->text(), error)) {
        showError(error);
        return;
    }
    accept();
}

void LoginDialog::onRegister()
{
    if (m_regPass->text() != m_regPass2->text()) {
        showError(QStringLiteral("两次输入的密码不一致"));
        return;
    }
    QString error;
    if (!AuthController::instance().registerUser(m_regUser->text(), m_regPass->text(),
                                                 QString(), error)) {
        showError(error);
        return;
    }
    accept();
}
