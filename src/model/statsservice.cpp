#include "statsservice.h"

#include <algorithm>

#include <QSet>

namespace {

bool inRangeAndMode(const Trip& t, const QDate& from, const QDate& to, const QString& modeFilter)
{
    const QDate day = t.startTime.toLocalTime().date();
    if (day < from || day > to)
        return false;
    if (!modeFilter.isEmpty() && t.modeCode != modeFilter)
        return false;
    return true;
}

} // namespace

namespace Stats {

Summary summarize(const QList<Trip>& trips, const QDate& from, const QDate& to,
                  const QString& modeFilter)
{
    Summary s;
    QSet<QString> stations;
    for (const Trip& t : trips) {
        if (!inRangeAndMode(t, from, to, modeFilter))
            continue;
        ++s.tripCount;
        if (t.distanceM)
            s.distanceM += *t.distanceM;
        if (t.costFen)
            s.costFen += *t.costFen;
        if (t.endTime.isValid())
            s.durationSec += (t.endTime.toMSecsSinceEpoch() - t.startTime.toMSecsSinceEpoch()) / 1000;
        if (!t.startPlace.trimmed().isEmpty())
            stations.insert(t.startPlace.trimmed());
        if (!t.endPlace.trimmed().isEmpty())
            stations.insert(t.endPlace.trimmed());
    }
    s.stationCount = stations.size();
    return s;
}

QList<MonthlyPoint> monthlyTrend(const QList<Trip>& trips, const QDate& from, const QDate& to,
                                 const QString& modeFilter)
{
    QHash<QDate, MonthlyPoint> map;
    for (const Trip& t : trips) {
        if (!inRangeAndMode(t, from, to, modeFilter))
            continue;
        const QDate d = t.startTime.toLocalTime().date();
        const QDate month(d.year(), d.month(), 1);
        MonthlyPoint& p = map[month];
        p.month = month;
        if (t.distanceM)
            p.distanceM += *t.distanceM;
        if (t.costFen)
            p.costFen += *t.costFen;
    }

    QList<MonthlyPoint> out;
    QDate m(from.year(), from.month(), 1);
    const QDate last(to.year(), to.month(), 1);
    while (m <= last) {
        const auto it = map.constFind(m);
        out.append(it == map.constEnd() ? MonthlyPoint{m} : it.value());
        m = m.addMonths(1);
    }
    return out;
}

QList<ModeShare> modeShares(const QList<Trip>& trips, const QDate& from, const QDate& to,
                            const QString& modeFilter)
{
    QHash<QString, int> map;
    for (const Trip& t : trips) {
        if (!inRangeAndMode(t, from, to, modeFilter))
            continue;
        ++map[t.modeCode];
    }
    QList<ModeShare> out;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        out.append({it.key(), it.value()});
    std::sort(out.begin(), out.end(),
              [](const ModeShare& a, const ModeShare& b) { return a.count > b.count; });
    return out;
}

} // namespace Stats
