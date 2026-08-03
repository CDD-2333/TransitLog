#pragma once

#include <QList>
#include <QWidget>

#include "repo/triprepository.h"

class QFlowLayout;
class QLabel;

// 图鉴：车次 / 车型 两段卡片墙（流式布局自动换行）。
// 卡片 = 名称 + 乘坐次数 + 首次/最近乘坐日期。数据由外部注入，本组件不做 SQL。
class VehicleCatalogWidget : public QWidget {
    Q_OBJECT
public:
    explicit VehicleCatalogWidget(QWidget* parent = nullptr);

    void refresh(const QList<TripRepository::VehicleStat>& numbers,
                 const QList<TripRepository::VehicleStat>& models);

private:
    QWidget* buildCard(const TripRepository::VehicleStat& stat);
    void rebuildWall();

    QList<TripRepository::VehicleStat> m_numbers;
    QList<TripRepository::VehicleStat> m_models;
    QFlowLayout* m_numLayout = nullptr;
    QFlowLayout* m_modelLayout = nullptr;
    QLabel* m_numEmpty = nullptr;
    QLabel* m_modelEmpty = nullptr;
};
