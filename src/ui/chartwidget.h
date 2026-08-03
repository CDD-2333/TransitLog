#pragma once

#include <QPointF>
#include <QVector>
#include <QWidget>

// 简易折线/面积图（QPainter 自绘，避免引入 Qt Charts 模块）。
// 输入：points[i].x = 第 i 天，points[i].y = 当日里程(km)。空数据安全。
class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget* parent = nullptr);

    void setSeries(const QVector<QPointF>& points);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<QPointF> m_points;
};
