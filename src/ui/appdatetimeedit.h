#pragma once

#include <QDateTimeEdit>

class QCalendarWidget;

// 时间选择器：去掉内置箭头与 up/down 按钮（QSS 隐藏），点击输入框任意位置弹出日历。
// 注意：QDateTimeEdit 一旦被 QSS 样式化（圆角/边框/背景），其内置 calendarPopup 的
// 内部触发区即失效（QStyleSheetStyle 处理 QAbstractSpinBox 的已知问题），
// 故此处手动把 calendarWidget() 作为 Qt::Popup 弹出，不依赖内置行为。
class AppDateTimeEdit : public QDateTimeEdit {
    Q_OBJECT
public:
    explicit AppDateTimeEdit(QWidget* parent = nullptr);
    explicit AppDateTimeEdit(const QDateTime& datetime, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void connectCalendar();
    void showCalendarPopup();
};
