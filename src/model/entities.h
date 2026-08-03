#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <optional>

// ==================== 领域实体（纯数据结构，无 QObject） ====================

struct User {
    QString id;
    QString username;
    QString nickname;
    QDateTime createdAt;
};

struct TransportMode {
    QString code;      // 稳定业务键，如 "SUBWAY"
    QString name;      // 中文名
    QString icon;      // emoji 图标
    int sortOrder = 0;
    bool isActive = true;
};

struct TripTag {
    QString code;
    QString name;
    QString color;     // #RRGGBB
    int sortOrder = 0;
    bool isActive = true;
};

struct Trip {
    QString id;
    QString userId;
    QString modeCode;

    QDateTime startTime;                 // 恒有效
    QDateTime endTime;                   // invalid => 进行中
    QString startPlace;
    QString endPlace;

    std::optional<double> startLat;      // 未填为 nullopt
    std::optional<double> startLng;
    std::optional<double> endLat;
    std::optional<double> endLng;

    std::optional<qint64> distanceM;     // 米
    std::optional<qint64> costFen;       // 分
    QString tagId;                       // 空串 => 无标签
    QString vehicleNo;                   // 车次/航班号：302路、K262次、CA1234（空串=未填）
    QString vehicleModel;                // 车型：CR400AF、DF4D（空串=未填）
    QString vehicleCar;                  // 车号：列车车厢号/车牌号（空串=未填）
    QString note;

    bool isDeleted = false;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isInProgress() const { return !endTime.isValid(); }
};

Q_DECLARE_METATYPE(Trip)
Q_DECLARE_METATYPE(User)
