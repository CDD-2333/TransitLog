#include "triprepository.h"

#include <algorithm>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include "app/databasemanager.h"

namespace {

QDateTime nowUtc()
{
    return QDateTime::currentDateTimeUtc();
}

// 把可选数值绑定为 QVariant()（NULL）或具体值
void bindOpt(QSqlQuery& q, const std::optional<qint64>& v)
{
    q.addBindValue(v ? QVariant::fromValue(*v) : QVariant());
}

void bindOpt(QSqlQuery& q, const std::optional<double>& v)
{
    q.addBindValue(v ? QVariant::fromValue(*v) : QVariant());
}

// 文本绑定：默认构造的 QString() 是 null，SQLite 驱动会绑成 SQL NULL，
// 而 NOT NULL 列（start_place 等）不接受 NULL —— 统一归一化为空串。
void bindText(QSqlQuery& q, const QString& s)
{
    q.addBindValue(s.isNull() ? QStringLiteral("") : s);
}

} // namespace

QList<Trip> TripRepository::tripsForUser(const QString& userId) const
{
    QList<Trip> out;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT id, user_id, mode_code, start_time, end_time, start_place, end_place,"
        " start_lat, start_lng, end_lat, end_lng, distance_m, cost_fen, tag_id,"
        " vehicle_no, vehicle_model, vehicle_car, note, created_at, updated_at"
        " FROM trip_record WHERE user_id = ? AND is_deleted = 0 ORDER BY start_time DESC"));
    q.addBindValue(userId);
    if (!q.exec())
        return out;

    while (q.next()) {
        Trip t;
        t.id = q.value(0).toString();
        t.userId = q.value(1).toString();
        t.modeCode = q.value(2).toString();
        t.startTime = QDateTime::fromMSecsSinceEpoch(q.value(3).toLongLong(), Qt::UTC);
        if (!q.value(4).isNull())
            t.endTime = QDateTime::fromMSecsSinceEpoch(q.value(4).toLongLong(), Qt::UTC);
        t.startPlace = q.value(5).toString();
        t.endPlace = q.value(6).toString();
        if (!q.value(7).isNull())
            t.startLat = q.value(7).toDouble();
        if (!q.value(8).isNull())
            t.startLng = q.value(8).toDouble();
        if (!q.value(9).isNull())
            t.endLat = q.value(9).toDouble();
        if (!q.value(10).isNull())
            t.endLng = q.value(10).toDouble();
        if (!q.value(11).isNull())
            t.distanceM = q.value(11).toLongLong();
        if (!q.value(12).isNull())
            t.costFen = q.value(12).toLongLong();
        t.tagId = q.value(13).toString();
        t.vehicleNo = q.value(14).toString();
        t.vehicleModel = q.value(15).toString();
        t.vehicleCar = q.value(16).toString();
        t.note = q.value(17).toString();
        t.createdAt = QDateTime::fromMSecsSinceEpoch(q.value(18).toLongLong(), Qt::UTC);
        t.updatedAt = QDateTime::fromMSecsSinceEpoch(q.value(19).toLongLong(), Qt::UTC);
        out.append(t);
    }
    return out;
}

QList<TransportMode> TripRepository::transportModes() const
{
    QList<TransportMode> out;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT code, name, icon, sort_order FROM transport_mode"
        " WHERE is_active = 1 ORDER BY sort_order"));
    if (!q.exec())
        return out;
    while (q.next()) {
        TransportMode m;
        m.code = q.value(0).toString();
        m.name = q.value(1).toString();
        m.icon = q.value(2).toString();
        m.sortOrder = q.value(3).toInt();
        out.append(m);
    }
    return out;
}

QList<TripTag> TripRepository::tripTags() const
{
    QList<TripTag> out;
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT code, name, color, sort_order FROM trip_tag"
        " WHERE is_active = 1 ORDER BY sort_order"));
    if (!q.exec())
        return out;
    while (q.next()) {
        TripTag t;
        t.code = q.value(0).toString();
        t.name = q.value(1).toString();
        t.color = q.value(2).toString();
        t.sortOrder = q.value(3).toInt();
        out.append(t);
    }
    return out;
}

bool TripRepository::contains(const QSqlDatabase& db, const QString& id) const
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT 1 FROM trip_record WHERE id = ?"));
    q.addBindValue(id);
    return q.exec() && q.next();
}

bool TripRepository::saveTrip(Trip& trip)
{
    QSqlDatabase db = DatabaseManager::instance().db();
    const bool exists = !trip.id.isEmpty() && contains(db, trip.id);
    const QDateTime now = nowUtc();

    if (trip.id.isEmpty())
        trip.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!trip.createdAt.isValid())
        trip.createdAt = now;
    trip.updatedAt = now;

    const auto bindEndTime = [&](QSqlQuery& query) {
        if (trip.isInProgress())
            query.addBindValue(QVariant());
        else
            query.addBindValue(trip.endTime.toMSecsSinceEpoch());
    };

    if (exists) {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE trip_record SET mode_code=?, start_time=?, end_time=?,"
            " start_place=?, end_place=?, start_lat=?, start_lng=?, end_lat=?, end_lng=?,"
            " distance_m=?, cost_fen=?, tag_id=?, vehicle_no=?, vehicle_model=?,"
            " vehicle_car=?, note=?, is_deleted=0, updated_at=? WHERE id=?"));
        q.addBindValue(trip.modeCode);
        q.addBindValue(trip.startTime.toMSecsSinceEpoch());
        bindEndTime(q);
        bindText(q, trip.startPlace);
        bindText(q, trip.endPlace);
        bindOpt(q, trip.startLat);
        bindOpt(q, trip.startLng);
        bindOpt(q, trip.endLat);
        bindOpt(q, trip.endLng);
        bindOpt(q, trip.distanceM);
        bindOpt(q, trip.costFen);
        q.addBindValue(trip.tagId.isEmpty() ? QVariant() : QVariant(trip.tagId));
        bindText(q, trip.vehicleNo);
        bindText(q, trip.vehicleModel);
        bindText(q, trip.vehicleCar);
        bindText(q, trip.note);
        q.addBindValue(trip.updatedAt.toMSecsSinceEpoch());
        q.addBindValue(trip.id);
        return q.exec();
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO trip_record (id, user_id, mode_code, start_time, end_time,"
        " start_place, end_place, start_lat, start_lng, end_lat, end_lng,"
        " distance_m, cost_fen, tag_id, vehicle_no, vehicle_model, vehicle_car,"
        " note, is_deleted, created_at, updated_at)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?)"));
    q.addBindValue(trip.id);
    q.addBindValue(trip.userId);
    q.addBindValue(trip.modeCode);
    q.addBindValue(trip.startTime.toMSecsSinceEpoch());
    bindEndTime(q);
    bindText(q, trip.startPlace);
    bindText(q, trip.endPlace);
    bindOpt(q, trip.startLat);
    bindOpt(q, trip.startLng);
    bindOpt(q, trip.endLat);
    bindOpt(q, trip.endLng);
    bindOpt(q, trip.distanceM);
    bindOpt(q, trip.costFen);
    q.addBindValue(trip.tagId.isEmpty() ? QVariant() : QVariant(trip.tagId));
    bindText(q, trip.vehicleNo);
    bindText(q, trip.vehicleModel);
    bindText(q, trip.vehicleCar);
    bindText(q, trip.note);
    q.addBindValue(trip.createdAt.toMSecsSinceEpoch());
    q.addBindValue(trip.updatedAt.toMSecsSinceEpoch());
    return q.exec();
}

QList<TripRepository::VehicleStat> TripRepository::vehicleStats(const QString& userId,
                                                                VehicleDim dim) const
{
    QList<VehicleStat> out;
    // 列名来自枚举白名单，杜绝注入
    const QString col = (dim == VehicleDim::Number) ? QStringLiteral("vehicle_no")
                                                    : QStringLiteral("vehicle_model");
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT %1, COUNT(*), MIN(start_time), MAX(start_time)"
        " FROM trip_record"
        " WHERE user_id = ? AND is_deleted = 0 AND %1 IS NOT NULL AND %1 != ''"
        " GROUP BY %1 ORDER BY COUNT(*) DESC").arg(col));
    q.addBindValue(userId);
    if (!q.exec())
        return out;
    while (q.next()) {
        VehicleStat s;
        s.name = q.value(0).toString();
        s.count = q.value(1).toInt();
        s.firstDate = QDateTime::fromMSecsSinceEpoch(q.value(2).toLongLong(), Qt::UTC);
        s.lastDate = QDateTime::fromMSecsSinceEpoch(q.value(3).toLongLong(), Qt::UTC);
        out.append(s);
    }
    return out;
}

bool TripRepository::softDeleteTrip(const QString& id)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "UPDATE trip_record SET is_deleted = 1, updated_at = ? WHERE id = ?"));
    q.addBindValue(nowUtc().toMSecsSinceEpoch());
    q.addBindValue(id);
    return q.exec();
}

bool TripRepository::hardDeleteAllForUser(const QString& userId)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral("DELETE FROM trip_record WHERE user_id = ?"));
    q.addBindValue(userId);
    return q.exec();
}

QList<TripRepository::DailyStat> TripRepository::dailyStats(const QString& userId,
                                                            const QDate& from,
                                                            const QDate& to) const
{
    QHash<QDate, DailyStat> map;
    const QList<Trip> trips = tripsForUser(userId);
    for (const Trip& t : trips) {
        const QDate day = QDateTime::fromMSecsSinceEpoch(t.startTime.toMSecsSinceEpoch()).date();
        if (day < from || day > to)
            continue;
        DailyStat& s = map[day];
        s.date = day;
        ++s.count;
        if (t.distanceM)
            s.distanceM += *t.distanceM;
        if (t.costFen)
            s.costFen += *t.costFen;
    }
    return map.values();
}

QList<TripRepository::ModeStat> TripRepository::modeStats(const QString& userId) const
{
    QHash<QString, ModeStat> map;
    const QList<Trip> trips = tripsForUser(userId);
    for (const Trip& t : trips) {
        ModeStat& s = map[t.modeCode];
        s.modeCode = t.modeCode;
        ++s.count;
        if (t.distanceM)
            s.distanceM += *t.distanceM;
    }
    QList<ModeStat> out = map.values();
    std::sort(out.begin(), out.end(),
              [](const ModeStat& a, const ModeStat& b) { return a.count > b.count; });
    return out;
}
