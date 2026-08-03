#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

#include "model/entities.h"

class QComboBox;
class QLabel;
class ChartWidget;
class QVBoxLayout;

// 统计页：汇总指标 + 每日里程折线 + 交通方式占比。
// 只接收 Trip 列表在内存中聚合，不碰 SQL。
class StatsWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatsWidget(QWidget* parent = nullptr);

    void setTrips(const QList<Trip>& trips);   // 全量（已过滤软删除）
    void setModes(const QHash<QString, TransportMode>& modes);
    void refresh();                            // 按当前时间范围重算

private slots:
    void onRangeChanged();

private:
    void buildUI();
    void recompute();
    void clearLayout(QLayout* layout);

    QList<Trip> m_trips;
    QHash<QString, TransportMode> m_modes;
    QComboBox* m_rangeCombo = nullptr;
    QLabel* m_totalDistance = nullptr;
    QLabel* m_totalCost = nullptr;
    QLabel* m_totalCount = nullptr;
    ChartWidget* m_chart = nullptr;
    QVBoxLayout* m_modeRows = nullptr;
};
