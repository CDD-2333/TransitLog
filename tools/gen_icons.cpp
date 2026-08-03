// 图标生成器（一次性工具，不属于主程序构建）
// 用 Tabler 图标字体渲染 QComboBox 线条箭头 PNG，并绘制应用占位图标。
// 用法: 编译后从项目根目录运行（需能读到 resources/fonts/tabler-icons.ttf）
//   g++ -std=c++17 tools/gen_icons.cpp -I$QT/include -I$QT/include/QtGui \
//       -L$QT/lib -lQt6Gui -lQt6Core -o /tmp/gen_icons && /tmp/gen_icons
#include <QCoreApplication>
#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {

// 用 Tabler 字形画一个图标 PNG
void drawTablerGlyph(const QString& fontFile, ushort codepoint, const QColor& color,
                     const QString& outPath, int size = 32)
{
    const int id = QFontDatabase::addApplicationFont(fontFile);
    if (id < 0) {
        qCritical() << "字体加载失败:" << fontFile;
        return;
    }
    const QString fam = QFontDatabase::applicationFontFamilies(id).first();

    QFont font(fam);
    font.setPixelSize(size);
    font.setStyleStrategy(QFont::NoAntialias); // 线条图标用抗锯齿更平滑
    font.setStyleStrategy(QFont::PreferAntialias);

    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setFont(font);
    p.setPen(color);
    p.drawText(pm.rect(), Qt::AlignCenter, QString(QChar(codepoint)));
    p.end();
    if (!pm.save(outPath))
        qCritical() << "保存失败:" << outPath;
    else
        qInfo() << "已生成" << outPath << pm.size();
}

// 应用占位图标：绿底圆角方块 + 白色路线折线 + 公交字形（公共交通 × 路线记录）
void drawAppIcon(const QString& fontFile, const QString& outPath, int size = 128)
{
    const int id = QFontDatabase::addApplicationFont(fontFile);
    if (id < 0) {
        qCritical() << "字体加载失败:" << fontFile;
        return;
    }
    const QString fam = QFontDatabase::applicationFontFamilies(id).first();

    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor bg(0x4A, 0x7C, 0x6F); // 主题主色
    const QColor fg(0xFF, 0xFF, 0xFF);

    // 圆角底
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(QRectF(4, 4, size - 8, size - 8), 24, 24);

    // 路线折线（路线记录元素）
    p.setPen(QPen(fg, 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath route;
    route.moveTo(28, 88);
    route.lineTo(48, 62);
    route.lineTo(66, 80);
    route.lineTo(100, 34);
    p.drawPath(route);

    // 公交字形（公共交通元素），叠在右下角
    QFont font(fam);
    font.setPixelSize(52);
    p.setFont(font);
    p.setPen(fg);
    p.drawText(QRectF(24, 44, 80, 80), Qt::AlignRight | Qt::AlignBottom,
               QString(QChar(0xebe4))); // bus

    p.end();
    if (!pm.save(outPath))
        qCritical() << "保存失败:" << outPath;
    else
        qInfo() << "已生成" << outPath << pm.size();
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("GenIcons");

    // QComboBox 箭头：浅色(#6B7280) / 深色(#9AA0A8) = 主题 text-secondary token。
    // 直接生成 16px（与 QSS 中 down-arrow 尺寸一致），避免缩放下采样冲淡颜色。
    drawTablerGlyph(QStringLiteral("resources/fonts/tabler-icons.ttf"), 0xea5f, // chevron-down
                    QColor(0x6B, 0x72, 0x80),
                    QStringLiteral("resources/icons/chevron-down-light.png"), 16);
    drawTablerGlyph(QStringLiteral("resources/fonts/tabler-icons.ttf"), 0xea5f,
                    QColor(0x9A, 0xA0, 0xA8),
                    QStringLiteral("resources/icons/chevron-down-dark.png"), 16);

    drawAppIcon(QStringLiteral("resources/fonts/tabler-icons.ttf"),
                QStringLiteral("resources/icons/app-icon.png"));
    return 0;
}
