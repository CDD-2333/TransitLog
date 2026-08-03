#include "chartwidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

#include "app/thememanager.h"

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
}

void ChartWidget::setSeries(const QVector<QPointF>& points)
{
    m_points = points;
    update();
}

void ChartWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto pal = ThemeManager::instance().palette(ThemeManager::instance().currentTheme());
    const QRect area = rect().adjusted(8, 12, -8, -8);

    if (m_points.isEmpty()) {
        p.setPen(QColor(pal.textSecondary));
        p.drawText(area, Qt::AlignCenter, QStringLiteral("暂无数据"));
        return;
    }

    double ymax = 0.0;
    for (const QPointF& pt : m_points)
        ymax = qMax(ymax, pt.y());
    if (ymax <= 0.0)
        ymax = 1.0;

    // 横向网格线
    p.setPen(QPen(QColor(pal.border), 1));
    const int lines = 4;
    for (int i = 0; i <= lines; ++i) {
        const qreal y = area.bottom() - area.height() * qreal(i) / lines;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }

    const auto xFor = [&](int i) {
        if (m_points.size() == 1)
            return qreal(area.left()) + area.width() / 2.0;
        return qreal(area.left()) + area.width() * qreal(i) / qreal(m_points.size() - 1);
    };
    const auto yFor = [&](qreal v) {
        return area.bottom() - area.height() * (v / ymax);
    };

    // 面积填充
    QPainterPath fill;
    for (int i = 0; i < m_points.size(); ++i) {
        const QPointF pt(xFor(i), yFor(m_points.at(i).y()));
        if (i == 0)
            fill.moveTo(pt);
        else
            fill.lineTo(pt);
    }
    fill.lineTo(QPointF(xFor(m_points.size() - 1), area.bottom()));
    fill.lineTo(QPointF(xFor(0), area.bottom()));
    fill.closeSubpath();
    QColor fillColor(pal.primary);
    fillColor.setAlpha(40);
    p.fillPath(fill, fillColor);

    // 折线
    QPainterPath path;
    for (int i = 0; i < m_points.size(); ++i) {
        const QPointF pt(xFor(i), yFor(m_points.at(i).y()));
        if (i == 0)
            path.moveTo(pt);
        else
            path.lineTo(pt);
    }
    p.setPen(QPen(QColor(pal.primary), 2));
    p.drawPath(path);

    // 数据点
    p.setBrush(QColor(pal.primary));
    p.setPen(Qt::NoPen);
    for (int i = 0; i < m_points.size(); ++i)
        p.drawEllipse(QPointF(xFor(i), yFor(m_points.at(i).y())), 3, 3);

    // y 轴最大值标注
    p.setPen(QColor(pal.textSecondary));
    p.drawText(QRect(area.left(), 0, area.width(), 20),
               Qt::AlignRight | Qt::AlignTop,
               QString::number(qRound(ymax)) + QStringLiteral(" km"));
}
