#include "appdatetimeedit.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>

#include "app/thememanager.h"

void AppDateTimeEdit::paintEvent(QPaintEvent* event)
{
    QDateTimeEdit::paintEvent(event);

    // 右侧按钮区绘制线条 chevron（与 QComboBox 下拉箭头同源 PNG，颜色=text-secondary token）
    const bool dark = (ThemeManager::instance().currentTheme() == Theme::Dark);
    const QPixmap arrow(dark ? QStringLiteral(":/resources/icons/chevron-down-dark.png")
                             : QStringLiteral(":/resources/icons/chevron-down-light.png"));
    if (arrow.isNull())
        return;

    const int s = 16;
    const int rightMargin = 12;  // 与输入框右侧 padding 一致，贴右缘放置
    const int x = width() - rightMargin - s;
    const int y = (height() - s) / 2;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPixmap(x, y, arrow);
}
