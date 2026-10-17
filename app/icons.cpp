#include "icons.h"

#include <QGuiApplication>
#include <QIconEngine>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QPolygonF>
#include <QtMath>

namespace {

constexpr int kVectorSet = 3;  // the "Modern" set: drawn in code, not read from a bitmap strip
int g_set = 0;

const char* const kFiles[] = {":/Scrows.bmp", ":/Windows XP.bmp", ":/Windows XP Large.bmp"};

struct Strip {
    QImage image;
    int cell = 16;
};

const Strip& strip(int set) {
    static Strip cache[3];
    static bool loaded[3] = {false, false, false};
    if (!loaded[set]) {
        QImage img = QImage(QString::fromLatin1(kFiles[set])).convertToFormat(QImage::Format_ARGB32);
        const QRgb key = img.isNull() ? 0 : (img.pixel(0, 0) & 0xFFFFFF);  // top-left pixel is the transparent key
        for (int y = 0; y < img.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x)
                if ((line[x] & 0xFFFFFF) == key) line[x] = 0;
        }
        cache[set].cell = img.height();  // strips are one row of square icons
        cache[set].image = img;
        loaded[set] = true;
    }
    return cache[set];
}

// --- the "Modern" set: line icons on a 24 x 24 grid ------------------------------------------------------------------
// Drawn at the size asked for, so they stay sharp on any screen scaling, follow the colours of the theme (light or dark)
// and fade when disabled. A few icons carry a coloured badge (green +, red x) or a coloured body (Go, Stop).
void drawVectorIcon(QPainter& p, int index, QIcon::Mode mode) {
    const QPalette pal = QGuiApplication::palette();
    const bool off = mode == QIcon::Disabled;
    const QColor fg = off ? pal.color(QPalette::Disabled, QPalette::ButtonText) : pal.color(QPalette::ButtonText);
    const QColor face = pal.color(QPalette::Button);  // hides what is behind a filled shape
    const QColor green = off ? fg : QColor(0x2e, 0x9e, 0x5b);
    const QColor red = off ? fg : QColor(0xe0, 0x4f, 0x45);
    const QPen pen(fg, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    auto line = [&](qreal a, qreal b, qreal c, qreal d) { p.drawLine(QPointF(a, b), QPointF(c, d)); };
    auto open = [&](std::initializer_list<QPointF> pts) { p.drawPolyline(QPolygonF(pts)); };
    auto closed = [&](std::initializer_list<QPointF> pts) { p.drawPolygon(QPolygonF(pts)); };
    auto page = [&](bool fill = false, Qt::PenStyle style = Qt::SolidLine) {  // a sheet with a folded corner
        QPen pp = pen;
        pp.setStyle(style);
        p.setPen(pp);
        if (fill) p.setBrush(face);
        closed({{6, 3}, {14, 3}, {19, 8}, {19, 21}, {6, 21}});
        p.setBrush(Qt::NoBrush);
        open({{14, 3}, {14, 8}, {19, 8}});
        p.setPen(pen);
    };
    auto folder = [&] { closed({{3, 19}, {3, 6}, {9, 6}, {11.5, 8.5}, {21, 8.5}, {21, 19}}); };
    auto badge = [&](const QColor& c, int kind) {  // 0 plus, 1 cross, 2 minus
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(QPointF(17.5, 17.5), 5.4, 5.4);
        p.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap));
        if (kind == 0) { line(17.5, 14.8, 17.5, 20.2); line(14.8, 17.5, 20.2, 17.5); }
        else if (kind == 1) { line(15.2, 15.2, 19.8, 19.8); line(19.8, 15.2, 15.2, 19.8); }
        else line(14.8, 17.5, 20.2, 17.5);
        p.restore();
    };
    auto list = [&] { line(3.5, 7, 13.5, 7); line(3.5, 12, 13.5, 12); line(3.5, 17, 9, 17); };
    auto floppy = [&] {
        p.drawRoundedRect(QRectF(4, 4, 16, 16), 2.4, 2.4);
        p.drawRect(QRectF(8, 4, 7, 5));
        p.drawRect(QRectF(7.5, 13, 9, 7));
    };
    auto trash = [&] {
        line(4.5, 7, 19.5, 7);
        open({{9, 7}, {9, 4}, {15, 4}, {15, 7}});
        closed({{6.5, 7}, {7.5, 20.5}, {16.5, 20.5}, {17.5, 7}});
        line(10.5, 11, 10.5, 17);
        line(13.5, 11, 13.5, 17);
    };
    auto glyphCircle = [&](const QColor& c) { p.setPen(QPen(c, 1.8)); p.drawEllipse(QPointF(12, 12), 9, 9); p.setPen(pen); };

    switch (index) {
    case TbTabFiles: page(); line(9, 13, 16, 13); line(9, 16.5, 16, 16.5); break;
    case TbTabActions:
        line(4, 7, 20, 7); line(4, 12, 20, 12); line(4, 17, 20, 17);
        p.setBrush(face);
        p.drawEllipse(QPointF(9, 7), 2.3, 2.3);
        p.drawEllipse(QPointF(16, 12), 2.3, 2.3);
        p.drawEllipse(QPointF(8, 17), 2.3, 2.3);
        break;
    case TbTabLog:
        p.drawRoundedRect(QRectF(5, 3.5, 14, 17), 2.2, 2.2);
        line(8.5, 9, 15.5, 9); line(8.5, 13, 15.5, 13); line(8.5, 17, 12.5, 17);
        break;
    case TbAddFiles: p.save(); p.translate(-2, -1.5); page(); p.restore(); badge(green, 0); break;
    case TbAddFolder: p.save(); p.translate(-2, -2); folder(); p.restore(); badge(green, 0); break;
    case TbRemoveSelected: p.save(); p.translate(-2, -1.5); page(); p.restore(); badge(red, 1); break;
    case TbRemoveAll:
        p.save(); p.translate(-4.5, -2.5); page(); p.restore();
        p.save(); p.translate(-2, -0.5); page(true); p.restore();
        badge(red, 1);
        break;
    case TbRemoveDead: p.save(); p.translate(-2, -1.5); page(false, Qt::DashLine); p.restore(); badge(red, 1); break;
    case TbPreview: {  // refresh arrows around a small eye
        const QRectF ring(3, 3, 18, 18);
        for (const int start : {160, -20}) {
            QPainterPath arc;
            arc.arcMoveTo(ring, start);
            arc.arcTo(ring, start, -130);
            p.drawPath(arc);
            const qreal a = (start - 130) * M_PI / 180;  // end of the arc, arrowhead along the clockwise tangent
            const QPointF e(12 + 9 * qCos(a), 12 - 9 * qSin(a)), d(qSin(a), qCos(a));
            for (const qreal turn : {0.7, -0.7}) {
                const qreal c = qCos(turn), s = qSin(turn);
                p.drawLine(e, e + QPointF(-d.x() * c + d.y() * s, -d.x() * s - d.y() * c) * 3.6);
            }
        }
        p.setPen(QPen(fg, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath eye;
        eye.moveTo(7.2, 12);
        eye.cubicTo(9, 8.8, 15, 8.8, 16.8, 12);
        eye.cubicTo(15, 15.2, 9, 15.2, 7.2, 12);
        p.drawPath(eye);
        p.setPen(Qt::NoPen);
        p.setBrush(fg);
        p.drawEllipse(QPointF(12, 12), 1.5, 1.5);
        break;
    }
    case TbMoveUp: open({{6, 15}, {12, 9}, {18, 15}}); break;
    case TbMoveDown: open({{6, 9}, {12, 15}, {18, 9}}); break;
    case TbMoveTop: line(6, 5, 18, 5); open({{6, 17}, {12, 11}, {18, 17}}); break;
    case TbMoveBottom: line(6, 19, 18, 19); open({{6, 7}, {12, 13}, {18, 7}}); break;
    case TbBatchAdd: list(); badge(green, 0); break;
    case TbBatchRemoveSel: list(); badge(red, 1); break;
    case TbBatchClear: trash(); break;
    case TbBatchOpen:
        open({{3, 19}, {3, 6}, {9, 6}, {11.5, 8.5}, {19, 8.5}, {19, 11}});
        closed({{3, 19}, {6, 11}, {22, 11}, {19, 19}});
        break;
    case TbBatchSave: floppy(); break;
    case TbLogClear: glyphCircle(fg); line(9, 9, 15, 15); line(15, 9, 9, 15); break;
    case TbLogSave: floppy(); break;
    case TbLogCopy:
        p.drawRoundedRect(QRectF(4, 3.5, 11.5, 12.5), 2.2, 2.2);
        p.setBrush(face);
        p.drawRoundedRect(QRectF(8.5, 8, 11.5, 12.5), 2.2, 2.2);
        break;
    case TbLogFilter: closed({{3, 5}, {21, 5}, {14, 13}, {14, 20}, {10, 18}, {10, 13}}); break;
    case TbGo:
        p.setPen(Qt::NoPen);
        p.setBrush(off ? QColor(fg.red(), fg.green(), fg.blue(), 140) : green);
        p.drawEllipse(QPointF(12, 12), 10, 10);
        p.setBrush(Qt::white);
        p.drawPolygon(QPolygonF({{9.8, 7.6}, {17, 12}, {9.8, 16.4}}));
        break;
    case TbStop:
        p.setPen(Qt::NoPen);
        p.setBrush(off ? QColor(fg.red(), fg.green(), fg.blue(), 140) : red);
        p.drawEllipse(QPointF(12, 12), 10, 10);
        p.setBrush(Qt::white);
        p.drawRoundedRect(QRectF(8.2, 8.2, 7.6, 7.6), 1.4, 1.4);
        break;
    case TbUndo: {
        open({{9, 6.5}, {4.5, 11}, {9, 15.5}});
        QPainterPath arc;
        arc.moveTo(4.5, 11);
        arc.lineTo(14, 11);
        arc.cubicTo(18.5, 11, 20.5, 13.5, 20.5, 16);
        arc.cubicTo(20.5, 18.5, 18.5, 20.5, 15, 20.5);
        arc.lineTo(11, 20.5);
        p.drawPath(arc);
        break;
    }
    case TbAlwaysOnTop:
        closed({{8, 3}, {16, 3}, {15, 10}, {18.5, 14}, {5.5, 14}, {9, 10}});
        line(12, 14, 12, 21.5);
        break;
    case TbOptions: {  // a real gear: eight trapezoid teeth around a body with a hole
        QPolygonF gear;
        auto at = [&](qreal deg, qreal r) { gear << QPointF(12 + r * qCos(deg * M_PI / 180), 12 - r * qSin(deg * M_PI / 180)); };
        for (int k = 0; k < 8; ++k) {
            const qreal c = k * 45;
            at(c - 15, 7.6); at(c - 9, 10.4); at(c + 9, 10.4); at(c + 15, 7.6);
        }
        p.drawPolygon(gear);
        p.drawEllipse(QPointF(12, 12), 3.1, 3.1);
        break;
    }
    case TbHelp: {
        glyphCircle(fg);
        QPainterPath q;
        q.moveTo(9.2, 9.4);
        q.cubicTo(9.2, 7.9, 10.4, 6.9, 12, 6.9);
        q.cubicTo(13.7, 6.9, 14.8, 8, 14.8, 9.3);
        q.cubicTo(14.8, 11.4, 12, 11.4, 12, 13.7);
        p.drawPath(q);
        p.setPen(Qt::NoPen);
        p.setBrush(fg);
        p.drawEllipse(QPointF(12, 17), 1.1, 1.1);
        break;
    }
    case TbAbout:  // an "i" in a speech bubble
        p.drawRoundedRect(QRectF(3.5, 3.5, 17, 13.5), 3.2, 3.2);
        open({{7.5, 17}, {7.5, 21}, {11.8, 17}});
        line(12, 10.4, 12, 13.6);
        p.setPen(Qt::NoPen);
        p.setBrush(fg);
        p.drawEllipse(QPointF(12, 7.6), 1.15, 1.15);
        break;
    case TbExit:
        open({{10, 4}, {5, 4}, {5, 20}, {10, 20}});
        p.setPen(QPen(red, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        line(10, 12, 20, 12);
        open({{16, 8}, {20, 12}, {16, 16}});
        break;
    default: break;
    }
}

class VectorIconEngine : public QIconEngine {
public:
    explicit VectorIconEngine(int index) : index_(index) {}
    QIconEngine* clone() const override { return new VectorIconEngine(index_); }
    void paint(QPainter* p, const QRect& r, QIcon::Mode mode, QIcon::State) override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->translate(r.topLeft());
        const qreal s = qMin(r.width(), r.height()) / 24.0;
        p->scale(s, s);
        drawVectorIcon(*p, index_, mode);
        p->restore();
    }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap pm(size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        paint(&p, QRect(QPoint(), size), mode, state);
        return pm;
    }

private:
    int index_;
};

}  // namespace

void setIconSet(int set) { g_set = (set >= 0 && set < iconSetCount()) ? set : 0; }
int iconSetCount() { return 4; }
QStringList iconSetNames() {
    return {QStringLiteral("Scrows"), QStringLiteral("Windows XP"), QStringLiteral("Windows XP Large"), QStringLiteral("Modern")};
}
int iconCell() { return g_set == kVectorSet ? 20 : strip(g_set).cell; }

QIcon tbIcon(int index) {
    if (g_set == kVectorSet) return QIcon(new VectorIconEngine(index));
    const Strip& s = strip(g_set);
    return QIcon(QPixmap::fromImage(s.image.copy(index * s.cell, 0, s.cell, s.cell)));
}
