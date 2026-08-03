#pragma once

#include <QObject>
#include <QString>

enum class Theme { Light, Dark };

// 主题 token 体系：浅色"纸感" + 深色 两套 token。
// 所有 QSS 只从 palette 拼装，不在各处硬编码颜色；切换主题全局刷 styleSheet。
class ThemeManager : public QObject {
    Q_OBJECT
public:
    struct Palette {
        QString bg, surface, surfaceAlt, border;
        QString primary, primaryHover, primarySoft;
        QString text, textSecondary;
        QString danger, dangerSoft;
        QString success, focus;
    };

    static ThemeManager& instance();

    Palette palette(Theme theme) const;
    QString buildQSS(Theme theme) const;

    Theme currentTheme() const;
    void setTheme(Theme theme);   // 持久化到 QSettings 并发射 themeChanged

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);

    Theme m_theme = Theme::Light;
};
