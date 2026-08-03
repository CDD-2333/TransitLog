#include "statswidget.h"

#include <algorithm>

#include <QComboBox>
#include <QDate>
#include "ui/chartwidget.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "model/format.h"

namespace {

struct Range {
    QDate from;
    QDate to;
    QString label;
};

Range rangeFor(int index)
{
    const QDate today = QDate::currentDate();
    switch (index) {
    case 0: // 近7天
        return {today.addDays(-6), today, QStringLiteral("近7天")};
    case 1: // 近30天
        return {today.addDays(-29), today, QStringLiteral("近30天")};
    case 2: // 本月
        return {QDate(today.year(), today.month(), 1), today, QStringLiteral("本月")};
    default: // 全部
        return {QDate(1970, 1, 1), today, QStringLiteral("全部")};
    }
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

    // 标题行
    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("统计"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    header->addWidget(title);
    header->addStretch();
    m_rangeCombo = new QComboBox(this);
    m_rangeCombo->addItems({QStringLiteral("近7天"), QStringLiteral("近30天"),
                            QStringLiteral("本月"), QStringLiteral("全部")});
    header->addWidget(m_rangeCombo);
    root->addLayout(header);
    connect(m_rangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsWidget::onRangeChanged);

    // 三栏指标卡
    auto* cards = new QHBoxLayout;
    cards->setSpacing(14);
    const QStringList captions = {QStringLiteral("总里程"), QStringLiteral("总费用"), QStringLiteral("行程数")};
    QLabel* values[3];
    values[0] = m_totalDistance = new QLabel(QStringLiteral("--"), this);
    values[1] = m_totalCost = new QLabel(QStringLiteral("--"), this);
    values[2] = m_totalCount = new QLabel(QStringLiteral("--"), this);
    for (int i = 0; i < 3; ++i) {
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

    // 每日里程折线
    auto* chartCard = new QFrame(this);
    chartCard->setObjectName(QStringLiteral("card"));
    auto* chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setContentsMargins(16, 12, 16, 12);
    auto* chartTitle = new QLabel(QStringLiteral("每日里程"), chartCard);
    chartTitle->setObjectName(QStringLiteral("hintLabel"));
    chartLayout->addWidget(chartTitle);
    m_chart = new ChartWidget(chartCard);
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
    refresh();
}

void StatsWidget::setModes(const QHash<QString, TransportMode>& modes)
{
    m_modes = modes;
}

void StatsWidget::onRangeChanged()
{
    refresh();
}

void StatsWidget::refresh()
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

    // 汇总
    qint64 dist = 0;
    qint64 cost = 0;
    int count = 0;
    QHash<QDate, qreal> daily;
    QHash<QString, int> modeCount;
    for (const Trip& t : m_trips) {
        const QDate day = t.startTime.toLocalTime().date();
        if (day < range.from || day > range.to)
            continue;
        ++count;
        if (t.distanceM) {
            dist += *t.distanceM;
            daily[day] += *t.distanceM / 1000.0;
        }
        if (t.costFen)
            cost += *t.costFen;
        ++modeCount[t.modeCode];
    }

    m_totalDistance->setText(dist > 0 ? Format::km(dist) : QStringLiteral("0 km"));
    m_totalCost->setText(cost > 0 ? Format::yuan(cost) : QStringLiteral("¥0.00"));
    m_totalCount->setText(QString::number(count));

    // 折线：把空日补 0，形成连续序列
    QVector<QPointF> points;
    points.reserve(range.from.daysTo(range.to) + 1);
    for (QDate d = range.from; d <= range.to; d = d.addDays(1))
        points.append(QPointF(points.size(), daily.value(d, 0.0)));
    m_chart->setSeries(points);

    // 交通方式占比
    clearLayout(m_modeRows);
    int totalCount = 0;
    for (int c : modeCount)
        totalCount += c;
    if (totalCount == 0) {
        auto* empty = new QLabel(QStringLiteral("暂无数据"), this);
        empty->setObjectName(QStringLiteral("hintLabel"));
        m_modeRows->addWidget(empty);
        return;
    }

    QList<QPair<QString, int>> list;
    for (auto it = modeCount.constBegin(); it != modeCount.constEnd(); ++it)
        list.append({it.key(), it.value()});
    std::sort(list.begin(), list.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) { return a.second > b.second; });

    for (const auto& pair : list) {
        const int percent = qRound(100.0 * pair.second / totalCount);
        auto* row = new QHBoxLayout;
        const QString modeName = m_modes.value(pair.first).name.isEmpty()
                                     ? pair.first
                                     : m_modes.value(pair.first).name;
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
