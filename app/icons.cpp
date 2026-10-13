#include "icons.h"

#include <QImage>
#include <QPixmap>

namespace {

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

}  // namespace

void setIconSet(int set) { g_set = (set >= 0 && set < iconSetCount()) ? set : 0; }
int iconSetCount() { return 3; }
QStringList iconSetNames() { return {QStringLiteral("Scrows"), QStringLiteral("Windows XP"), QStringLiteral("Windows XP Large")}; }
int iconCell() { return strip(g_set).cell; }

QIcon tbIcon(int index) {
    const Strip& s = strip(g_set);
    return QIcon(QPixmap::fromImage(s.image.copy(index * s.cell, 0, s.cell, s.cell)));
}
