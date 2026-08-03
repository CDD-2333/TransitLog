#include "statswidget.h"

#include <QComboBox>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "model/format.h"
#include "model/statsservice.h"
#include "ui/trendchartwidget.h"

namespace {

struct Range {
    QDate from;
    QDate to;
};

Range rangeFor(int index)
{
    const QDate today = QDate::currentDate();
    switch (index) {
    case 0: return {today.addDays(-6), today};                        // 近7天
    case 1: return {today.addDays(-29), today};                       // 近30天
    case 2: return {QDate(today.year(), today.month(), 1), today};    // 本月
    case 3: return {QDate(today.year(), 1, 1), today};                // 今年
    default: return {QDate(1970, 1, 1), today};                       // 全部
    }
}

QString fmtDuration(qint64 sec)
{
    if (sec <= 0)
        return QStringLiteral("0m");
    const qint64 d = sec / 86400;
    const qint64 h = (sec % 86400) / 3600;
    const qint64 m = (sec % 3600) / 60;
    if (d > 0)
        return QStringLiteral("%1天%2h").arg(d).arg(h);
    if (h > 0)
        return QStringLiteral("%1h%2m").arg(h).arg(m);
    return QStringLiteral("%1m").arg(m);
}

} // namespace

StatsWidget::StatsWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void StatsWidget::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 20);
    root->setSpacing(14);

    // 标题行 + 时间范围 + 交通方式筛选
    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("统计"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    header->addWidget(title);
    header->addStretch();
    m_modeCombo = new QComboBox(this);
    m_modeCombo->setMinimumWidth(130);
    header->addWidget(m_modeCombo);
    m_rangeCombo = new QComboBox(this);
    m_rangeCombo->addItems({QStringLiteral("近7天"), QStringLiteral("近30天"),
                            QStringLiteral("本月"), QStringLiteral("今年"), QStringLiteral("全部")});
    header->addWidget(m_rangeCombo);
    root->addLayout(header);
    connect(m_rangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsWidget::onRangeChanged);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsWidget::onModeChanged);

    // 四张核心数字卡片
    auto* cards = new QHBoxLayout;
    cards->setSpacing(14);
    const QStringList captions = {QStringLiteral("总里程"), QStringLiteral("总时长"),
                                  QStringLiteral("总花费"), QStringLiteral("到访站点")};
    QLabel* values[4];
    values[0] = m_totalDistance = new QLabel(QStringLiteral("--"), this);
    values[1] = m_totalDuration = new QLabel(QStringLiteral("--"), this);
    values[2] = m_totalCost = new QLabel(QStringLiteral("--"), this);
    values[3] = m_totalStations = new QLabel(QStringLiteral("--"), this);
    for (int i = 0; i < 4; ++i) {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("card"));
        auto* v = new QVBoxLayout(card);
        v->setContentsMargins(16, 14, 16, 14);
        auto* cap = new QLabel(captions.at(i), card);
        cap->setObjectName(QStringLiteral("summaryCaption"));
        values[i]->setObjectName(QStringLiteral("summaryValue"));
        v->addWidget(values[i]);
        v->addWidget(cap);
        cards->addWidget(card, 1);
    }
    root->addLayout(cards);

    // 月度趋势图（Qt Charts）
    auto* chartCard = new QFrame(this);
    chartCard->setObjectName(QStringLiteral("card"));
    auto* chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setContentsMargins(16, 12, 16, 12);
    auto* chartTitle = new QLabel(QStringLiteral("月度趋势（里程/花费）"), chartCard);
    chartTitle->setObjectName(QStringLiteral("hintLabel"));
    chartLayout->addWidget(chartTitle);
    m_chart = new TrendChartWidget(chartCard);
    chartLayout->addWidget(m_chart);
    root->addWidget(chartCard);

    // 交通方式占比
    auto* modeCard = new QFrame(this);
    modeCard->setObjectName(QStringLiteral("card"));
    auto* modeLayout = new QVBoxLayout(modeCard);
    modeLayout->setContentsMargins(16, 12, 16, 12);
    auto* modeTitle = new QLabel(QStringLiteral("交通方式占比"), modeCard);
    modeTitle->setObjectName(QStringLiteral("hintLabel"));
    modeLayout->addWidget(modeTitle);
    m_modeRows = new QVBoxLayout;
    modeLayout->addLayout(m_modeRows);
    root->addWidget(modeCard);

    root->addStretch();
}

void StatsWidget::setTrips(const QList<Trip>& trips)
{
    m_trips = trips;
    recompute();
}

void StatsWidget::setModes(const QHash<QString, TransportMode>& modes)
{
    m_modes = modes;
    // 填充交通方式筛选（首次调用；保留"全部"）
    if (m_modeCombo->count() <= 1) {
        m_modeCombo->addItem(QStringLiteral("全部交通方式"), QString());
        for (auto it = modes.constBegin(); it != modes.constEnd(); ++it)
            m_modeCombo->addItem(it.value().name, it.key());
    }
}

void StatsWidget::onRangeChanged()
{
    recompute();
}

void StatsWidget::onModeChanged()
{
    recompute();
}

void StatsWidget::clearLayout(QLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        if (QLayout* child = item->layout())
            clearLayout(child);
        delete item;
    }
}

void StatsWidget::recompute()
{
    const Range range = rangeFor(m_rangeCombo->currentIndex());
    const QString modeFilter = m_modeCombo->currentData().toString();

    const Stats::Summary sum = Stats::summarize(m_trips, range.from, range.to, modeFilter);

    m_totalDistance->setText(sum.distanceM > 0 ? Format::km(sum.distanceM)
                                               : QStringLiteral("0 km"));
    m_totalDuration->setText(fmtDuration(sum.durationSec));
    m_totalCost->setText(sum.costFen > 0 ? Format::yuan(sum.costFen) : QStringLiteral("¥0.00"));
    m_totalStations->setText(QString::number(sum.stationCount));

    m_chart->setMonthlyData(Stats::monthlyTrend(m_trips, range.from, range.to, modeFilter));

    // 交通方式占比
    clearLayout(m_modeRows);
    const auto shares = Stats::modeShares(m_trips, range.from, range.to, modeFilter);
    int totalCount = 0;
    for (const auto& s : shares)
        totalCount += s.count;
    if (totalCount == 0) {
        auto* empty = new QLabel(QStringLiteral("暂无数据"), this);
        empty->setObjectName(QStringLiteral("hintLabel"));
        m_modeRows->addWidget(empty);
        return;
    }
    for (const auto& s : shares) {
        const int percent = qRound(100.0 * s.count / totalCount);
        auto* row = new QHBoxLayout;
        const QString modeName = m_modes.value(s.modeCode).name.isEmpty()
                                     ? s.modeCode
                                     : m_modes.value(s.modeCode).name;
        auto* name = new QLabel(modeName, this);
        name->setObjectName(QStringLiteral("modeNameLabel"));
        auto* bar = new QProgressBar(this);
        bar->setRange(0, 100);
        bar->setValue(percent);
        bar->setTextVisible(false);
        bar->setFixedHeight(8);
        auto* pct = new QLabel(QStringLiteral("%1%").arg(percent), this);
        pct->setObjectName(QStringLiteral("modePercentLabel"));
        row->addWidget(name);
        row->addWidget(bar, 1);
        row->addWidget(pct);
        m_modeRows->addLayout(row);
    }
}
