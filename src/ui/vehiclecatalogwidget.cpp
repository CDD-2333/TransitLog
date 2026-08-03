#include "vehiclecatalogwidget.h"

#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "app/thememanager.h"
#include "ui/qflowlayout.h"

namespace {

void clearLayout(QLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        if (QLayout* child = item->layout())
            clearLayout(child);
        delete item;
    }
}

QString fmtDate(const QDateTime& dt)
{
    return dt.isValid() ? dt.toLocalTime().date().toString(QStringLiteral("yyyy-MM-dd"))
                        : QStringLiteral("--");
}

} // namespace

VehicleCatalogWidget::VehicleCatalogWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("catalogPage"));   // 配合 QSS：容器背景透明，由窗口主题色透出
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 20);
    root->setSpacing(12);

    // ---- 车次图鉴 ----
    auto* numTitle = new QLabel(QStringLiteral("车次图鉴"), this);
    numTitle->setObjectName(QStringLiteral("pageTitle"));
    root->addWidget(numTitle);
    m_numEmpty = new QLabel(QStringLiteral("暂无车次记录（填了「车次」的行程才会出现在这里）"), this);
    m_numEmpty->setObjectName(QStringLiteral("hintLabel"));
    m_numEmpty->setWordWrap(true);
    root->addWidget(m_numEmpty);

    auto* numScroll = new QScrollArea(this);
    numScroll->setWidgetResizable(true);
    numScroll->setFrameShape(QFrame::NoFrame);
    numScroll->viewport()->setAutoFillBackground(false);   // 透明：由窗口主题色透出
    auto* numContainer = new QWidget;
    numContainer->setAutoFillBackground(false);
    m_numLayout = new QFlowLayout(numContainer, 0, 12, 12);
    numScroll->setWidget(numContainer);
    root->addWidget(numScroll, 1);

    // ---- 车型图鉴 ----
    auto* modelTitle = new QLabel(QStringLiteral("车型图鉴"), this);
    modelTitle->setObjectName(QStringLiteral("pageTitle"));
    root->addWidget(modelTitle);
    m_modelEmpty = new QLabel(QStringLiteral("暂无车型记录（填了「车型」的行程才会出现在这里）"), this);
    m_modelEmpty->setObjectName(QStringLiteral("hintLabel"));
    m_modelEmpty->setWordWrap(true);
    root->addWidget(m_modelEmpty);

    auto* modelScroll = new QScrollArea(this);
    modelScroll->setWidgetResizable(true);
    modelScroll->setFrameShape(QFrame::NoFrame);
    modelScroll->viewport()->setAutoFillBackground(false);
    auto* modelContainer = new QWidget;
    modelContainer->setAutoFillBackground(false);
    m_modelLayout = new QFlowLayout(modelContainer, 0, 12, 12);
    modelScroll->setWidget(modelContainer);
    root->addWidget(modelScroll, 1);
}

QWidget* VehicleCatalogWidget::buildCard(const TripRepository::VehicleStat& stat)
{
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("card"));
    card->setFixedWidth(180);

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 14);
    v->setSpacing(6);

    auto* name = new QLabel(stat.name, card);
    name->setObjectName(QStringLiteral("summaryValue"));   // 主色大字
    name->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 700;"));
    v->addWidget(name);

    auto* count = new QLabel(QStringLiteral("乘坐 %1 次").arg(stat.count), card);
    count->setObjectName(QStringLiteral("summaryCaption"));
    v->addWidget(count);

    auto* range = new QLabel(
        QStringLiteral("首次 %1\n最近 %2").arg(fmtDate(stat.firstDate), fmtDate(stat.lastDate)), card);
    range->setObjectName(QStringLiteral("hintLabel"));
    range->setWordWrap(true);
    v->addWidget(range);

    return card;
}

void VehicleCatalogWidget::rebuildWall()
{
    // 车次
    clearLayout(m_numLayout);
    m_numEmpty->setVisible(m_numbers.isEmpty());
    for (const auto& s : m_numbers)
        m_numLayout->addWidget(buildCard(s));

    // 车型
    clearLayout(m_modelLayout);
    m_modelEmpty->setVisible(m_models.isEmpty());
    for (const auto& s : m_models)
        m_modelLayout->addWidget(buildCard(s));
}

void VehicleCatalogWidget::refresh(const QList<TripRepository::VehicleStat>& numbers,
                                   const QList<TripRepository::VehicleStat>& models)
{
    m_numbers = numbers;
    m_models = models;
    rebuildWall();
}
