#include "theme.h"

#include <QApplication>
#include <QStyle>
#include <QToolTip>

bool isDarkPalette(const QPalette& p) { return p.color(QPalette::Base).lightness() < p.color(QPalette::Text).lightness(); }

static QPalette darkPalette() {
    QPalette p;
    const QColor window(0x2b, 0x2b, 0x2b), base(0x1e, 0x1e, 0x1e), button(0x38, 0x38, 0x38), text(0xe6, 0xe6, 0xe6);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::ToolTipBase, button);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor(0xff, 0x6b, 0x6b));
    p.setColor(QPalette::Link, QColor(0x6c, 0xb0, 0xff));
    p.setColor(QPalette::Highlight, QColor(0x2a, 0x82, 0xda));
    p.setColor(QPalette::HighlightedText, Qt::white);
    // shades derived from the button colour: the toolbar style sheet draws hover, pressed and borders with them
    p.setColor(QPalette::Light, button.lighter(160));
    p.setColor(QPalette::Midlight, QColor(0x46, 0x46, 0x46));
    p.setColor(QPalette::Mid, QColor(0x58, 0x58, 0x58));
    p.setColor(QPalette::Dark, QColor(0x18, 0x18, 0x18));
    p.setColor(QPalette::Shadow, Qt::black);
    p.setColor(QPalette::PlaceholderText, QColor(0x8a, 0x8a, 0x8a));
    const QColor dim(0x80, 0x80, 0x80);
    for (auto r : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) p.setColor(QPalette::Disabled, r, dim);
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0x50, 0x50, 0x50));
    return p;
}

// Written out, not taken from Fusion's standardPalette(): with Qt 6.4 that palette comes from the platform theme, so on a
// dark desktop (Linux Mint, say) "light" gave the dark colours of the desktop.
static QPalette lightPalette() {
    QPalette p;
    const QColor window(0xef, 0xef, 0xef), text(Qt::black);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, QColor(0xf7, 0xf7, 0xf7));
    p.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xdc));
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Link, QColor(0x00, 0x00, 0xff));
    p.setColor(QPalette::Highlight, QColor(0x30, 0x8c, 0xc6));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Light, Qt::white);
    p.setColor(QPalette::Midlight, QColor(0xca, 0xca, 0xca));
    p.setColor(QPalette::Mid, QColor(0xb8, 0xb8, 0xb8));
    p.setColor(QPalette::Dark, QColor(0x9f, 0x9f, 0x9f));
    p.setColor(QPalette::Shadow, QColor(0x76, 0x76, 0x76));
    p.setColor(QPalette::PlaceholderText, QColor(0x76, 0x76, 0x76));
    const QColor dim(0xbe, 0xbe, 0xbe);
    for (auto r : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) p.setColor(QPalette::Disabled, r, dim);
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0x91, 0x91, 0x91));
    return p;
}

void applyTheme(int mode, bool fusionWhenSystem) {
    static const QPalette systemPalette = QGuiApplication::palette();  // taken before the first change
    static const QString systemStyle = QApplication::style()->name();
    static const QPalette systemTip = QToolTip::palette();  // the desktop theme gives the tool tips a palette of their own
    if (mode == 1 || mode == 2) {
        QApplication::setStyle(QStringLiteral("Fusion"));
        const QPalette p = mode == 2 ? darkPalette() : lightPalette();
        QApplication::setPalette(p);
        // Without this a desktop in the other theme leaves e.g. white text on a pale yellow tool tip.
        QPalette tip = p;
        tip.setColor(QPalette::ToolTipBase, mode == 2 ? QColor(0x3c, 0x3c, 0x3c) : QColor(0xff, 0xff, 0xdc));
        tip.setColor(QPalette::ToolTipText, mode == 2 ? QColor(0xe6, 0xe6, 0xe6) : Qt::black);
        tip.setColor(QPalette::Window, tip.color(QPalette::ToolTipBase));
        tip.setColor(QPalette::WindowText, tip.color(QPalette::ToolTipText));
        tip.setColor(QPalette::Base, tip.color(QPalette::ToolTipBase));
        tip.setColor(QPalette::Text, tip.color(QPalette::ToolTipText));
        QToolTip::setPalette(tip);
    } else {
        QApplication::setStyle(fusionWhenSystem ? QStringLiteral("Fusion") : systemStyle);
        QApplication::setPalette(systemPalette);
        QToolTip::setPalette(systemTip);
    }
}
