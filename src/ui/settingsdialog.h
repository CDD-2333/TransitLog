#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QPushButton;

// 设置页：主题切换、本地账户（改密/退出）、数据管理（备份导出/清空）。
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    void dataCleared();        // 本地行程被清空，主窗口需刷新
    void logoutRequested();    // 用户点击退出登录

private slots:
    void onThemeChanged(int index);
    void onChangePassword();
    void onExportBackup();
    void onClearData();
    void onLogout();

private:
    void buildUI();

    QComboBox* m_themeCombo = nullptr;
    QLabel* m_userLabel = nullptr;
    QLabel* m_dbPathLabel = nullptr;
};
