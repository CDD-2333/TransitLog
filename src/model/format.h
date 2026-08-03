#pragma once

#include <QString>

// 展示层格式化：库内存整数最小单位，显示时再换算，避免浮点误差。
namespace Format {

inline QString km(qint64 meters)
{
    if (meters < 1000)
        return QString::number(meters) + QStringLiteral(" m");
    return QString::number(meters / 1000.0, 'f', 1) + QStringLiteral(" km");
}

inline QString yuan(qint64 fen)
{
    return QStringLiteral("¥") + QString::number(fen / 100.0, 'f', 2);
}

} // namespace Format
