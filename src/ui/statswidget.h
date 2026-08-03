#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

#include "model/entities.h"

class QComboBox;
class QLabel;
class QVBoxLayout;
class TrendChartWidget;

// 统计页：核心数字卡片 + 月度趋势图(Qt Charts) + 交通方式占比。
// 只接收 Trip 列表，聚合计算全部走 Stats::Service，不碰 SQL。
class StatsWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatsWidget(QWidget* parent = nullptr);

    void setTrips(const QList<Trip>& trips);
    void setModes(const QHash<QString, TransportMode>& modes);

private slots:
    void onRangeChanged();
    void onModeChanged();

private:
    void buildUI();
    void recompute();
    void clearLayout(QLayout* layout);

    QList<Trip> m_trips;
    QHash<QString, TransportMode> m_modes;
    QComboBox* m_rangeCombo = nullptr;
    QComboBox* m_modeCombo = nullptr;
    QLabel* m_totalDistance = nullptr;
    QLabel* m_totalDuration = nullptr;
    QLabel* m_totalCost = nullptr;
    QLabel* m_totalStations = nullptr;
    TrendChartWidget* m_chart = nullptr;
    QVBoxLayout* m_modeRows = nullptr;
};
