#pragma once

#include <QStyledItemDelegate>

// 行程卡片渲染：圆角卡片 + emoji 图标 + 标签chip + 进行中徽标。
// 颜色从 ThemeManager palette 读取（与 QSS 同一 token 来源）。
class TripCardDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit TripCardDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void drawChip(QPainter* painter, const QString& text, const QColor& color,
                  const QRect& base) const;
};
