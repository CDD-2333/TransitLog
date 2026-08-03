#pragma once

#include <QAbstractListModel>
#include <QHash>

#include "model/entities.h"

// 行程列表 Model：包装 Trip 列表，供 QListView + 自定义 delegate 消费。
// 不做任何 SQL，数据由外部（Repository）注入。
class TripListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum TripRole {
        ModeIconRole = Qt::UserRole + 1,
        ModeNameRole,
        RouteTextRole,   // "海淀黄庄 → 北京大学东门"
        TimeTextRole,    // "08:12 – 08:31" / "08:12 – 进行中"
        MetaTextRole,    // "3.2 km · ¥4.00" / "--"
        NoteRole,
        TagTextRole,
        TagColorRole,
        InProgressRole,
        TripRole         // 完整 Trip
    };

    explicit TripListModel(QObject* parent = nullptr);

    void setTrips(const QList<Trip>& trips);
    void setModes(const QHash<QString, TransportMode>& modes);
    void setTags(const QHash<QString, TripTag>& tags);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Trip tripAt(int row) const;

private:
    QList<Trip> m_trips;
    QHash<QString, TransportMode> m_modes;
    QHash<QString, TripTag> m_tags;
};
