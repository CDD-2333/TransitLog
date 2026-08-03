#include "tripcarddelegate.h"

#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOptionViewItem>

#include "app/thememanager.h"
#include "model/triplistmodel.h"

TripCardDelegate::TripCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QSize TripCardDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    const bool hasNote = !index.data(TripListModel::NoteRole).toString().isEmpty();
    return QSize(260, hasNote ? 96 : 78);
}

void TripCardDelegate::drawChip(QPainter* painter, const QString& text, const QColor& color,
                                const QRect& base) const
{
    QFont f = painter->font();
    f.setPixelSize(11);
    painter->setFont(f);
    const int w = painter->fontMetrics().horizontalAdvance(text) + 12;
    const QRect r(base.x(), base.y(), w, 18);
    painter->setPen(Qt::NoPen);
    QColor bg = color;
    bg.setAlpha(32);
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 9, 9);
    painter->setPen(color);
    painter->drawText(r, Qt::AlignCenter, text);
}

void TripCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto pal = ThemeManager::instance().palette(ThemeManager::instance().currentTheme());
    const QRect r = option.rect.adjusted(2, 3, -2, -3);

    QColor bg = QColor(pal.surface);
    if (option.state & QStyle::State_MouseOver)
        bg = QColor(pal.surfaceAlt);
    if (option.state & QStyle::State_Selected)
        bg = QColor(pal.primarySoft);

    painter->setPen(QPen(QColor(pal.border), 1));
    painter->setBrush(bg);
    painter->drawRoundedRect(r, 10, 10);

    const int left = r.left() + 14;
    const int top = r.top() + 12;

    // emoji 图标
    const QString icon = index.data(TripListModel::ModeIconRole).toString();
    QFont iconFont = option.font;
    iconFont.setPixelSize(24);
    painter->setFont(iconFont);
    painter->setPen(QColor(pal.text));
    painter->drawText(QRect(left, top, 34, 30), Qt::AlignCenter,
                      icon.isEmpty() ? QStringLiteral("🚩") : icon);

    const int tx = left + 40;

    // 第一行：交通方式名 + 标签chip
    const QString modeName = index.data(TripListModel::ModeNameRole).toString();
    QFont nameFont = option.font;
    nameFont.setBold(true);
    painter->setFont(nameFont);
    painter->setPen(QColor(pal.text));
    const QRect nameRect(tx, top, r.width() - 90, 20);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, modeName);

    const QString tagName = index.data(TripListModel::TagTextRole).toString();
    if (!tagName.isEmpty()) {
        const QColor tagColor(index.data(TripListModel::TagColorRole).toString());
        const int chipX = tx + painter->fontMetrics().horizontalAdvance(modeName) + 8;
        drawChip(painter, tagName, tagColor, QRect(chipX, top + 1, 60, 18));
    }

    // 第二行：路线
    painter->setFont(option.font);
    painter->setPen(QColor(pal.textSecondary));
    painter->drawText(QRect(tx, top + 22, r.width() - 70, 18),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      index.data(TripListModel::RouteTextRole).toString());

    // 第三行：时间 · 元数据
    const QString time = index.data(TripListModel::TimeTextRole).toString();
    const QString meta = index.data(TripListModel::MetaTextRole).toString();
    painter->drawText(QRect(tx, top + 42, r.width() - 70, 18),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      time + QStringLiteral("  ·  ") + meta);

    // 进行中徽标（右上角）
    if (index.data(TripListModel::InProgressRole).toBool()) {
        QFont bf = option.font;
        bf.setPixelSize(11);
        bf.setBold(true);
        painter->setFont(bf);
        const QString badge = QStringLiteral("进行中");
        const int bw = painter->fontMetrics().horizontalAdvance(badge) + 16;
        const QRect br(r.right() - bw - 10, r.top() + 10, bw, 20);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(pal.primarySoft));
        painter->drawRoundedRect(br, 10, 10);
        painter->setPen(QColor(pal.primary));
        painter->drawText(br, Qt::AlignCenter, badge);
    }

    // 备注（第四行，有才显示）
    const QString note = index.data(TripListModel::NoteRole).toString();
    if (!note.isEmpty()) {
        painter->setFont(option.font);
        painter->setPen(QColor(pal.textSecondary));
        painter->drawText(QRect(tx, top + 60, r.width() - 70, 16),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          QStringLiteral("备注: ") + note);
    }

    painter->restore();
}
