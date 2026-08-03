#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QMessageBox>

#include "app/databasemanager.h"
#include "app/session.h"
#include "app/thememanager.h"
#include "ui/logindialog.h"
#include "ui/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TransitLog"));
    QApplication::setOrganizationName(QStringLiteral("CDD-2333"));
    QApplication::setApplicationDisplayName(QStringLiteral("TransitLog 行程记录"));

    // Tabler 图标字体：交通方式图标 / 主题、设置按钮等（替代 emoji）
    QFontDatabase::addApplicationFont(QStringLiteral(":/resources/fonts/tabler-icons.ttf"));
    // 应用图标占位（公共交通 × 路线记录）
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/resources/icons/app-icon.png")));

    if (!DatabaseManager::instance().open()) {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"),
                              QStringLiteral("无法打开本地数据库：\n%1")
                                  .arg(DatabaseManager::instance().lastError()));
        return 1;
    }

    app.setStyleSheet(ThemeManager::instance().buildQSS(ThemeManager::instance().currentTheme()));

    // 本地登录：未登录先走登录/注册
    if (!Session::instance().isLoggedIn()) {
        LoginDialog login;
        if (login.exec() != QDialog::Accepted)
            return 0;
    }

    MainWindow w;
    w.show();
    return app.exec();
}
