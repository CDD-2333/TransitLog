#include "triplistmodel.h"

#include "model/format.h"

TripListModel::TripListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void TripListModel::setTrips(const QList<Trip>& trips)
{
    beginResetModel();
    m_trips = trips;
    endResetModel();
}

void TripListModel::setModes(const QHash<QString, TransportMode>& modes)
{
    m_modes = modes;
}

void TripListModel::setTags(const QHash<QString, TripTag>& tags)
{
    m_tags = tags;
}

int TripListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_trips.size();
}

QVariant TripListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_trips.size())
        return QVariant();

    const Trip& t = m_trips.at(index.row());
    switch (role) {
    case ModeIconRole:
        return m_modes.value(t.modeCode).icon;
    case ModeNameRole:
        return m_modes.value(t.modeCode).name;
    case RouteTextRole: {
        const QString start = t.startPlace.trimmed();
        const QString end = t.endPlace.trimmed();
        QString route;
        if (!start.isEmpty() && !end.isEmpty())
            route = start + QStringLiteral(" → ") + end;
        else if (!start.isEmpty())
            route = start;
        else if (!end.isEmpty())
            route = end;
        return route.isEmpty() ? QStringLiteral("(未填地点)") : route;
    }
    case TimeTextRole: {
        const QString start = t.startTime.toLocalTime().toString(QStringLiteral("MM-dd HH:mm"));
        if (t.isInProgress())
            return start + QStringLiteral(" – 进行中");
        return start + QStringLiteral(" – ") + t.endTime.toLocalTime().toString(QStringLiteral("HH:mm"));
    }
    case MetaTextRole: {
        QStringList parts;
        if (t.distanceM)
            parts << Format::km(*t.distanceM);
        if (t.costFen)
            parts << Format::yuan(*t.costFen);
        return parts.isEmpty() ? QStringLiteral("--") : parts.join(QStringLiteral(" · "));
    }
    case NoteRole:
        return t.note;
    case TagTextRole:
        return m_tags.value(t.tagId).name;
    case TagColorRole:
        return m_tags.value(t.tagId).color;
    case InProgressRole:
        return t.isInProgress();
    case TripRole:
        return QVariant::fromValue(t);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TripListModel::roleNames() const
{
    return {
        {ModeIconRole, "modeIcon"},
        {ModeNameRole, "modeName"},
        {RouteTextRole, "routeText"},
        {TimeTextRole, "timeText"},
        {MetaTextRole, "metaText"},
        {NoteRole, "note"},
        {TagTextRole, "tagText"},
        {TagColorRole, "tagColor"},
        {InProgressRole, "inProgress"},
        {TripRole, "trip"},
    };
}

Trip TripListModel::tripAt(int row) const
{
    if (row < 0 || row >= m_trips.size())
        return Trip();
    return m_trips.at(row);
}
