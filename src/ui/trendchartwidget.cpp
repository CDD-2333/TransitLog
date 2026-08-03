#include "trendchartwidget.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QVBoxLayout>

#include "app/thememanager.h"

TrendChartWidget::TrendChartWidget(QWidget* parent)
    : QWidget(parent)
{
    m_chart = new QChart;
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_view = new QChartView(m_chart, this);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(220);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this](Theme) { rebuildChart(); });
}

void TrendChartWidget::setMonthlyData(const QList<Stats::MonthlyPoint>& points)
{
    m_points = points;
    rebuildChart();
}

void TrendChartWidget::rebuildChart()
{
    const auto pal = ThemeManager::instance().palette(ThemeManager::instance().currentTheme());

    m_chart->removeAllSeries();
    const auto axes = m_chart->axes();
    for (QAbstractAxis* ax : axes)
        m_chart->removeAxis(ax);

    if (m_points.isEmpty()) {
        m_chart->setTitle(QStringLiteral("暂无数据"));
        m_chart->setTitleBrush(QColor(pal.textSecondary));
        return;
    }

    QStringList cats;
    for (const Stats::MonthlyPoint& p : m_points)
        cats << p.month.toString(QStringLiteral("yy-MM"));

    // 里程折线（主色，左轴 km）
    auto* dist = new QLineSeries;
    dist->setName(QStringLiteral("里程 (km)"));
    dist->setPen(QPen(QColor(pal.primary), 2));
    for (int i = 0; i < m_points.size(); ++i)
        dist->append(i, m_points.at(i).distanceM / 1000.0);

    // 花费柱（金色，右轴 元）
    auto* costSet = new QBarSet(QStringLiteral("花费 (元)"));
    costSet->setColor(QColor(pal.accentGold));
    for (const Stats::MonthlyPoint& p : m_points)
        costSet->append(p.costFen / 100.0);
    auto* cost = new QBarSeries;
    cost->append(costSet);
    cost->setBarWidth(0.6);

    // 双 Y 轴
    auto* axisX = new QBarCategoryAxis;
    axisX->append(cats);
    axisX->setLabelsColor(QColor(pal.textSecondary));

    auto* axisYLeft = new QValueAxis;
    axisYLeft->setTitleText(QStringLiteral("km"));
    axisYLeft->setLabelFormat(QStringLiteral("%d"));
    axisYLeft->setLabelsColor(QColor(pal.textSecondary));
    axisYLeft->setTitleBrush(QColor(pal.textSecondary));

    auto* axisYRight = new QValueAxis;
    axisYRight->setTitleText(QStringLiteral("元"));
    axisYRight->setLabelFormat(QStringLiteral("%d"));
    axisYRight->setLabelsColor(QColor(pal.textSecondary));
    axisYRight->setTitleBrush(QColor(pal.textSecondary));

    m_chart->addSeries(dist);
    m_chart->addSeries(cost);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisYLeft, Qt::AlignLeft);
    m_chart->addAxis(axisYRight, Qt::AlignRight);
    dist->attachAxis(axisX);
    dist->attachAxis(axisYLeft);
    cost->attachAxis(axisX);
    cost->attachAxis(axisYRight);

    m_chart->legend()->setLabelColor(QColor(pal.textSecondary));
    m_chart->setTitle(QString());
    m_chart->setBackgroundBrush(Qt::NoBrush);
    m_chart->setPlotAreaBackgroundVisible(false);
}
