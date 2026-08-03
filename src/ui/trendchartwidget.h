#pragma once

#include <QList>
#include <QWidget>

#include "model/statsservice.h"

class QChart;
class QChartView;
class QLineSeries;
class QBarSeries;

// 月度趋势图（Qt Charts）：里程折线(主色) + 花费柱(金色)，双 Y 轴。
// 颜色跟随主题 token；主题切换自动刷新。
class TrendChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrendChartWidget(QWidget* parent = nullptr);

    void setMonthlyData(const QList<Stats::MonthlyPoint>& points);

private:
    void rebuildChart();

    QChart* m_chart = nullptr;
    QChartView* m_view = nullptr;
    QLineSeries* m_distSeries = nullptr;
    QBarSeries* m_costSeries = nullptr;
    QList<Stats::MonthlyPoint> m_points;
};
