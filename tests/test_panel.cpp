// Round trip of the action editor: an action loaded into the controls (ActionPanel::setFromAction, used to edit
// an action of the plan) and read back (ActionPanel::makeAction) must be identical, for every action type.
//
// Every attribute the action saves is changed to a different value before the round trip, so a field added to
// an action (or a new action type) that setFromAction() does not know about makes this test fail.
#include "actionpanel.h"
#include "ren_actions.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QXmlStreamWriter>
#include <cstdio>

using namespace Qt::StringLiterals;
using namespace ren;

static QString serialize(const Action& a) {
    QByteArray b;
    QXmlStreamWriter w(&b);
    a.save(w);
    return QString::fromUtf8(b);
}

// Another valid value for every attribute of the saved action (the root element is left alone).
static QString mutated(const QString& xml) {
    static const QRegularExpression attr(u" ([A-Za-z]+)=\"([^\"]*)\""_s);
    const qsizetype from = xml.indexOf(u"<Batch>"_s);
    QString out = xml.left(from), tail = xml.mid(from);
    qsizetype last = 0;
    QString res;
    for (auto it = attr.globalMatch(tail); it.hasNext();) {
        const auto m = it.next();
        const QString name = m.captured(1), v = m.captured(2);
        QString nv;
        if (name == u"WhichDate") nv = u"0"_s;                                   // only 0 or 1 exist
        else if (v == u"True" || v == u"False") nv = v == u"True" ? u"False"_s : u"True"_s;
        else if (v == u"0") nv = u"1"_s;
        else if (!v.isEmpty() && v.at(0).isDigit()) nv = QString::number(v.toInt() + 1);  // stays above the minimum of 1
        else nv = v + u"x"_s;
        res += tail.mid(last, m.capturedStart(2) - last) + nv;
        last = m.capturedEnd(2);
    }
    return out + res + tail.mid(last);
}

int main(int argc, char** argv) {
#ifndef Q_OS_WIN  // no display on the Linux CI; on Windows the deployed "windows" plugin is the only one and nothing is shown
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
#endif
    QStandardPaths::setTestModeEnabled(true);  // never touch the real settings
    QApplication app(argc, argv);
    ActionPanel panel;
    int failures = 0;

    const int last = int(ActionType::Exif);
    if (ActionList::create(ActionType(last + 1))->type() != ActionType::Unknown) {
        std::fprintf(stderr, "FAIL: there is a new action type: add it to ActionPanel::makeAction/setFromAction and to this test\n");
        ++failures;
    }

    const QString file = QDir::temp().filePath(u"antrenamer-test-panel.arb"_s);
    for (int t = 0; t <= last; ++t) {
        ActionList defaults;
        defaults.add(ActionType(t));
        if (!defaults.save(file)) { std::fprintf(stderr, "FAIL type %d: cannot write %s\n", t, qPrintable(file)); ++failures; continue; }
        QFile in(file);
        in.open(QIODevice::ReadOnly);
        const QString xml = QString::fromUtf8(in.readAll());
        in.close();
        QFile out(file);
        out.open(QIODevice::WriteOnly);
        out.write(mutated(xml).toUtf8());
        out.close();

        ActionList list;
        if (!list.load(file) || list.size() != 1) { std::fprintf(stderr, "FAIL type %d: cannot load the changed action\n", t); ++failures; continue; }
        // the two actions that keep a text outside their attributes
        if (t == int(ActionType::MultstrRepl)) static_cast<ActionMultstrRepl&>(list.at(0)).setChars = u"a\tb\nc\td\n"_s;
        if (t == int(ActionType::FromList)) static_cast<ActionFromList&>(list.at(0)).listContents = u"x\ny"_s;

        panel.setFromAction(list.at(0));
        const auto back = panel.makeAction(false);
        const QString a = serialize(list.at(0)), b = back ? serialize(*back) : QString();
        if (a != b) {
            std::fprintf(stderr, "FAIL type %d:\n  loaded : %s\n  read back: %s\n", t, qPrintable(a), qPrintable(b));
            ++failures;
        }
    }
    QFile::remove(file);
    std::fprintf(stderr, failures ? "panel round trip: %d failure(s)\n" : "panel round trip: all %d action types OK\n", failures ? failures : last + 1);
    return failures ? 1 : 0;
}
