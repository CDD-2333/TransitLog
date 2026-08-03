#pragma once

#include <QDate>
#include <QList>
#include <QString>

#include "model/entities.h"

// 统计聚合 Service：所有汇总/分组逻辑集中于此（纯计算，不碰 SQL，不依赖 UI）。
// 时间范围用本地日期；数据来自 Trip 列表（由 Repository 注入）。
namespace Stats {

struct Summary {
    qint64 distanceM = 0;      // 总里程(米)
    qint64 durationSec = 0;    // 总时长(秒，不含进行中行程)
    qint64 costFen = 0;        // 总花费(分)
    int stationCount = 0;      // 到访去重站点数
    int tripCount = 0;         // 行程数
};

struct MonthlyPoint {
    QDate month;               // 当月首日
    qint64 distanceM = 0;
    qint64 costFen = 0;
};

struct ModeShare {
    QString modeCode;
    int count = 0;
};

// modeFilter 为空 = 全部交通方式；否则只看该 mode
Summary summarize(const QList<Trip>& trips, const QDate& from, const QDate& to,
                  const QString& modeFilter = QString());

// 月度趋势：覆盖 [from首月, to当月]，空月补 0
QList<MonthlyPoint> monthlyTrend(const QList<Trip>& trips, const QDate& from, const QDate& to,
                                 const QString& modeFilter = QString());

QList<ModeShare> modeShares(const QList<Trip>& trips, const QDate& from, const QDate& to,
                            const QString& modeFilter = QString());

} // namespace Stats
