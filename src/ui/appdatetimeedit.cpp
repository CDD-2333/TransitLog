#include "appdatetimeedit.h"

#include <QCalendarWidget>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>

AppDateTimeEdit::AppDateTimeEdit(QWidget* parent)
    : QDateTimeEdit(parent)
{
    setCalendarPopup(true);
    connectCalendar();
}

AppDateTimeEdit::AppDateTimeEdit(const QDateTime& datetime, QWidget* parent)
    : QDateTimeEdit(datetime, parent)
{
    setCalendarPopup(true);
    connectCalendar();
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
