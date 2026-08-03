#pragma once

#include <QDateTimeEdit>

// 时间选择器：QDateTimeEdit 的 QSS 下拉子控件无法稳定渲染箭头，
// 故在此子类中自行绘制与 QComboBox 同款的线条 chevron（颜色取主题 text-secondary token）。
class AppDateTimeEdit : public QDateTimeEdit {
    Q_OBJECT
public:
    using QDateTimeEdit::QDateTimeEdit;

protected:
    void paintEvent(QPaintEvent* event) override;
};
