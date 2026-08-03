#include "thememanager.h"

#include <QSettings>

namespace {

ThemeManager::Palette lightPalette()
{
    // 浅色"纸感"，延续 MemoRise 调色
    return {
        QStringLiteral("#F3F4F6"), // bg
        QStringLiteral("#FFFFFF"), // surface
        QStringLiteral("#FAFAF9"), // surfaceAlt
        QStringLiteral("#E8E6E2"), // border
        QStringLiteral("#4A7C6F"), // primary
        QStringLiteral("#3D6B5E"), // primaryHover
        QStringLiteral("#E8F5F0"), // primarySoft
        QStringLiteral("#2D2D2D"), // text
        QStringLiteral("#6B7280"), // textSecondary
        QStringLiteral("#E24B4A"), // danger
        QStringLiteral("#FDE8E8"), // dangerSoft
        QStringLiteral("#4A7C59"), // success
        QStringLiteral("#B0CCC0"), // focus
    };
}

ThemeManager::Palette darkPalette()
{
    // 深色：不是浅色反色，另选对比度足够的色
    return {
        QStringLiteral("#1B1D21"), // bg
        QStringLiteral("#24262C"), // surface
        QStringLiteral("#2C2F36"), // surfaceAlt
        QStringLiteral("#3A3D45"), // border
        QStringLiteral("#6FA98F"), // primary
        QStringLiteral("#7FBBA0"), // primaryHover
        QStringLiteral("#2C3B34"), // primarySoft
        QStringLiteral("#E6E6E6"), // text
        QStringLiteral("#9AA0A8"), // textSecondary
        QStringLiteral("#E4605F"), // danger
        QStringLiteral("#3A2B2B"), // dangerSoft
        QStringLiteral("#6FA98F"), // success
        QStringLiteral("#5A7D6E"), // focus
    };
}

} // namespace

ThemeManager& ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    QSettings s;
    const QString saved = s.value(QStringLiteral("theme/name"), QStringLiteral("light")).toString();
    m_theme = (saved == QLatin1String("dark")) ? Theme::Dark : Theme::Light;
}

Theme ThemeManager::currentTheme() const
{
    return m_theme;
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_theme == theme)
        return;
    m_theme = theme;
    QSettings s;
    s.setValue(QStringLiteral("theme/name"),
               (theme == Theme::Dark) ? QStringLiteral("dark") : QStringLiteral("light"));
    emit themeChanged(m_theme);
}

ThemeManager::Palette ThemeManager::palette(Theme theme) const
{
    return (theme == Theme::Dark) ? darkPalette() : lightPalette();
}

QString ThemeManager::buildQSS(Theme theme) const
{
    const Palette p = palette(theme);
    // 下拉箭头：线条 chevron，颜色 = 主题 text-secondary token（不新增颜色）
    const QString arrowUrl = (theme == Theme::Dark)
        ? QStringLiteral(":/resources/icons/chevron-down-dark.png")
        : QStringLiteral(":/resources/icons/chevron-down-light.png");
    QString qss = QStringLiteral(R"(
/* 基础 */
QWidget { color: %TEXT; font-size: 14px; }
QMainWindow, QDialog { background-color: %BG; }
QMessageBox { background-color: %SURFACE; }
QLabel { background: transparent; }
QLabel#appTitle { font-size: 18px; font-weight: 700; color: %PRIMARY; }
QLabel#pageTitle { font-size: 22px; font-weight: 700; color: %TEXT; }
QLabel#hintLabel { color: %TEXT2; font-size: 13px; }
QLabel#summaryValue { font-size: 24px; font-weight: 700; color: %PRIMARY; }
QLabel#summaryCaption { color: %TEXT2; font-size: 12px; }
QLabel#dbPathLabel { color: %TEXT2; font-size: 12px; }

/* 卡片 */
QFrame#card { background-color: %SURFACE; border: 1px solid %BORDER; border-radius: 12px; }
QWidget#topBar { background-color: %SURFACE; border-bottom: 1px solid %BORDER; }

/* 进度条（统计页交通方式占比） */
QProgressBar { border: none; background-color: %BORDER; border-radius: 4px; height: 8px; }
QProgressBar::chunk { background-color: %PRIMARY; border-radius: 4px; }

/* 按钮 */
QPushButton {
    background-color: %SURFACE; color: %PRIMARY;
    border: 1.5px solid %PRIMARY; border-radius: 8px;
    padding: 8px 18px; font-size: 14px; font-weight: 600;
}
QPushButton:hover { background-color: %PRIMARY_SOFT; }
QPushButton:pressed { background-color: %PRIMARY; color: %SURFACE; }
QPushButton#primaryButton { background-color: %PRIMARY; color: %SURFACE; border: none; }
QPushButton#primaryButton:hover { background-color: %PRIMARY_HOVER; }
QPushButton#primaryButton:disabled { background-color: #D4D8D6; color: #A0A8A4; }
QPushButton#dangerButton { background-color: transparent; color: %DANGER; border: 1.5px solid %DANGER; }
QPushButton#dangerButton:hover { background-color: %DANGER_SOFT; }
QPushButton#iconButton { background-color: transparent; border: none; color: %TEXT; padding: 6px 10px; }
QPushButton#iconButton:hover { background-color: %SURFACE_ALT; }
/* 顶栏工具按钮：圆角矩形 */
QPushButton#toolButton {
    background-color: %SURFACE_ALT;
    border: 1px solid %BORDER;
    border-radius: 14px;
    padding: 6px 16px;
    color: %TEXT2;
    font-size: 13px;
}
QPushButton#toolButton:hover { background-color: %PRIMARY_SOFT; border-color: %PRIMARY; color: %PRIMARY; }
QPushButton#toolButton:pressed { background-color: %PRIMARY; color: %SURFACE; }
QPushButton#navButton { background-color: transparent; border: none; color: %TEXT2; padding: 6px 14px; }
QPushButton#navButton:hover { color: %TEXT; }
QPushButton#navButton:checked { color: %PRIMARY; font-weight: 700; border-bottom: 2px solid %PRIMARY; border-radius: 0; }

/* 输入 */
QLineEdit, QComboBox, QDateTimeEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox {
    background-color: %SURFACE_ALT; border: 1.5px solid %BORDER; border-radius: 8px;
    padding: 8px 12px; color: %TEXT; selection-background-color: %PRIMARY_SOFT;
}
QLineEdit:focus, QComboBox:focus, QDateTimeEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
    border-color: %PRIMARY;
}
/* 下拉箭头：drop-down 区与主体融合（无独立背景块/分隔线），箭头用线条 chevron */
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 26px;
    border: none;
    background: transparent;
}
QComboBox::down-arrow {
    image: url(%ARROW_URL);
    width: 16px;
    height: 16px;
}
QComboBox::drop-down:on { border: none; background: transparent; }
QComboBox::down-arrow:on { image: url(%ARROW_URL); }
QComboBox QAbstractItemView {
    background-color: %SURFACE; border: 1px solid %BORDER; color: %TEXT;
    selection-background-color: %PRIMARY_SOFT; selection-color: %PRIMARY;
}

/* 时间选择器（QDateTimeEdit）：彻底隐藏 up/down 按钮与箭头，输入框呈纯净样式；
   点击任意位置由 AppDateTimeEdit 重定向到内部触发区弹出日历。 */
QDateTimeEdit::up-button { width: 0; height: 0; border: none; background: transparent; }
QDateTimeEdit::down-button { width: 0; height: 0; border: none; background: transparent; }
QDateTimeEdit::up-arrow { image: none; }
QDateTimeEdit::down-arrow { image: none; }
QDateTimeEdit::up-arrow:on { image: none; }
QDateTimeEdit::down-arrow:on { image: none; }

/* 列表 */
QListView { background-color: transparent; border: none; }
QListView::item { background: transparent; }
QScrollBar:vertical { background: transparent; width: 8px; }
QScrollBar::handle:vertical { background: %BORDER; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 8px; }
QScrollBar::handle:horizontal { background: %BORDER; border-radius: 4px; min-width: 30px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* 菜单 */
QMenu { background-color: %SURFACE; border: 1px solid %BORDER; border-radius: 8px; padding: 6px; }
QMenu::item { padding: 8px 24px; border-radius: 6px; color: %TEXT; }
QMenu::item:selected { background-color: %PRIMARY_SOFT; color: %PRIMARY; }

/* Tab（登录页） */
QTabWidget::pane { border: 1px solid %BORDER; border-radius: 8px; background: %SURFACE; }
QTabBar::tab { background: transparent; padding: 8px 20px; color: %TEXT2; border-bottom: 2px solid transparent; }
QTabBar::tab:selected { color: %PRIMARY; border-bottom: 2px solid %PRIMARY; font-weight: 600; }
QTabBar::tab:hover { color: %TEXT; }

/* 统计页 */
QWidget#statsPage { background-color: transparent; }
QLabel#modeNameLabel { color: %TEXT2; font-size: 13px; }
QLabel#modePercentLabel { color: %TEXT2; font-size: 12px; }
)");

    // token -> 实际颜色值。
    // 注意替换顺序：含前缀的复合 token（%PRIMARY_SOFT 等）必须先替换，
    // 否则 %PRIMARY 先被替换会导致 %PRIMARY_SOFT 残留成 "#4A7C6F_SOFT"。
    qss.replace(QStringLiteral("%ARROW_URL"), arrowUrl);
    qss.replace(QStringLiteral("%PRIMARY_SOFT"), p.primarySoft);
    qss.replace(QStringLiteral("%PRIMARY_HOVER"), p.primaryHover);
    qss.replace(QStringLiteral("%SURFACE_ALT"), p.surfaceAlt);
    qss.replace(QStringLiteral("%DANGER_SOFT"), p.dangerSoft);
    qss.replace(QStringLiteral("%TEXT2"), p.textSecondary);
    qss.replace(QStringLiteral("%PRIMARY"), p.primary);
    qss.replace(QStringLiteral("%SURFACE"), p.surface);
    qss.replace(QStringLiteral("%DANGER"), p.danger);
    qss.replace(QStringLiteral("%TEXT"), p.text);
    qss.replace(QStringLiteral("%BG"), p.bg);
    qss.replace(QStringLiteral("%BORDER"), p.border);
    return qss;
}
