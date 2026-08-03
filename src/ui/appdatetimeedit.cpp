#include "appdatetimeedit.h"

#include <QCalendarWidget>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>

#include "app/thememanager.h"

AppDateTimeEdit::AppDateTimeEdit(QWidget* parent)
    : QDateTimeEdit(parent)
{
    // NoButtons：从源头移除 up/down 按钮与内部按钮框，字段呈纯输入框样式，
    // 与 QLineEdit 外观一致；日历由手动 popup 提供。
    setCalendarPopup(true);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    connectCalendar();
}

AppDateTimeEdit::AppDateTimeEdit(const QDateTime& datetime, QWidget* parent)
    : QDateTimeEdit(datetime, parent)
{
    setCalendarPopup(true);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    connectCalendar();
}

void AppDateTimeEdit::paintEvent(QPaintEvent* event)
{
    // QDateTimeEdit 即使 NoButtons，QAbstractSpinBox 仍会在右缘画一条原生 sunken frame
    // 内线（视觉上与 QLineEdit 不一致）。这里只把右缘内线用主题底色盖掉，
    // 其余边框/聚焦边框交给 base + QSS 统一规则（与 QLineEdit 完全一致）。
    QDateTimeEdit::paintEvent(event);

    const auto pal = ThemeManager::instance().palette(ThemeManager::instance().currentTheme());
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(pal.surfaceAlt));
    p.drawRect(QRect(width() - 5, 1, 3, height() - 2));
}

void AppDateTimeEdit::connectCalendar()
{
    // 选择日期后写回并收起
    connect(calendarWidget(), &QCalendarWidget::clicked, this,
            [this](const QDate& d) {
                setDate(d);
                calendarWidget()->hide();
            });
}

void AppDateTimeEdit::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && calendarPopup()) {
        setFocus(Qt::MouseFocusReason);
        QCalendarWidget* cal = calendarWidget();
        if (cal && cal->isVisible()) {
            cal->hide();          // 已展开则收起（点击外部本就自动关闭）
        } else {
            showCalendarPopup();
        }
        event->accept();
        return;
    }
    QDateTimeEdit::mousePressEvent(event);
}

void AppDateTimeEdit::showCalendarPopup()
{
    QCalendarWidget* cal = calendarWidget();
    if (!cal)
        return;
    cal->setWindowFlag(Qt::Popup, true);   // 点击外部自动关闭
    cal->setSelectedDate(date());

    // 定位在输入框正下方
    QPoint g = mapToGlobal(QPoint(0, height() + 2));
    QScreen* screen = QGuiApplication::screenAt(g);
    if (screen) {
        const QRect avail = screen->availableGeometry();
        const QSize sz = cal->sizeHint();
        if (g.x() + sz.width() > avail.right())
            g.setX(qMax(avail.left(), avail.right() - sz.width()));
        if (g.y() + sz.height() > avail.bottom())
            g.setY(qMax(avail.top(), g.y() - sz.height() - height() - 4));
    }
    cal->move(g);
    cal->show();
    cal->raise();
    cal->activateWindow();
}
