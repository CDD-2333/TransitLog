#pragma once

#include <QDate>
#include <QList>

#include "model/entities.h"

class QSqlDatabase;

// 行程相关的全部 SQL 集中在这里。
class TripRepository {
public:
    struct DailyStat {
        QDate date;
        qint64 distanceM = 0;
        qint64 costFen = 0;
        int count = 0;
    };
    struct ModeStat {
        QString modeCode;
        int count = 0;
        qint64 distanceM = 0;
    };

    QList<Trip> tripsForUser(const QString& userId) const;
    QList<TransportMode> transportModes() const;
    QList<TripTag> tripTags() const;

    bool saveTrip(Trip& trip);                  // 新建或更新（按 id 判断）
    bool softDeleteTrip(const QString& id);
    bool hardDeleteAllForUser(const QString& userId);

    // 统计：给定 [from, to]（本地日期），按本地日聚合
    QList<DailyStat> dailyStats(const QString& userId, const QDate& from, const QDate& to) const;
    QList<ModeStat> modeStats(const QString& userId) const;

private:
    bool contains(const QSqlDatabase& db, const QString& id) const;
};
