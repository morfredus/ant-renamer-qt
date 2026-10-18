// Round trip of the action editor: an action loaded into the controls (ActionPanel::setFromAction, used to edit
// an action of the plan) and read back (ActionPanel::makeAction) must be identical, for every action type.
//
// Every attribute the action saves is changed to a different value before the round trip, so a field added to
// an action (or a new action type) that setFromAction() does not know about makes this test fail.
#include "actionpanel.h"
#include "lngtranslator.h"
#include "ren_actions.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
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


// Language files: the loader must cope with what editors and translators do to them.
static int lngChecks() {
    int bad = 0;
    auto expect = [&bad](const QString& got, const QString& want, const char* what) {
        if (got != want) {
            std::fprintf(stderr, "FAIL language loader, %s:\n  got : %s\n  want: %s\n", what, qPrintable(got), qPrintable(want));
            ++bad;
        }
    };
    QTemporaryDir dir;
    auto write = [&](const QString& name, const QByteArray& data) {
        QFile f(dir.filePath(name));
        if (f.open(QIODevice::WriteOnly)) f.write(data);
    };
    const QByteArray head = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Application><MainForm>\n";
    const QByteArray tail = "</MainForm></Application>\n";
    write(u"English.lng"_s, head + "<ActionGo Caption=\"Go\" Hint=\"Go|Start\"/>\n<Pos Caption=\" at position %s\"/>\n<Plain Caption=\"Plain\"/>\n" + tail);
    auto tr = [](const LngTranslator& l, const char* s) { return l.translate("Ren", s); };

    // a UTF-8 BOM (Notepad and friends add one) must not make the file unreadable
    write(u"Bom.lng"_s, QByteArray("\xEF\xBB\xBF") + head + "<ActionGo Caption=\"D\xC3\xA9marrer\" Hint=\"D\xC3\xA9marrer|Lancer\"/>\n<Pos Caption=\"\xC3\xA0 la position %s\"/>\n<Plain Caption=\"Simple\"/>\n" + tail);
    LngTranslator bom;
    expect(bom.loadLanguage(dir.path(), u"Bom"_s) ? u"loaded"_s : u"not loaded"_s, u"loaded"_s, "BOM file loads");
    expect(tr(bom, "Go"), u"D\u00e9marrer"_s, "BOM file: caption");
    expect(tr(bom, "Start"), u"Lancer"_s, "BOM file: second half of a hint");
    // the English text decides about the spaces glued around a piece of text
    expect(tr(bom, " at position %1"), u" \u00e0 la position %1"_s, "leading space kept, %s becomes %1");

    // declared UTF-8 but written in windows-1252: read as windows-1252 instead of showing replacement characters
    write(u"Latin.lng"_s, head + "<ActionGo Caption=\"D\xE9marrer\"/>\n" + tail);
    LngTranslator latin;
    latin.loadLanguage(dir.path(), u"Latin"_s);
    expect(tr(latin, "Go"), u"D\u00e9marrer"_s, "invalid UTF-8 falls back to windows-1252");

    // a damaged file (entity without its semicolon) still gives what was read before the damage
    write(u"Broken.lng"_s, head + "<Plain Caption=\"Simple\"/>\n<ActionGo Caption=\"Lancer &#34\"/>\n<Pos Caption=\"x\"/>\n" + tail);
    LngTranslator broken;
    broken.loadLanguage(dir.path(), u"Broken"_s);
    expect(tr(broken, "Plain"), u"Simple"_s, "damaged file: text before the error is kept");

    // the texts added by Ant Renamer Qt live in the real language files (section QtPort), not in the code
    LngTranslator fr;
    fr.loadLanguage(QStringLiteral(LNG_SOURCE_DIR), u"French"_s);
    expect(tr(fr, "Settings storage"), u"Stockage des réglages"_s, "real French.lng: added text");
    expect(tr(fr, "Modification of action %1"), u"Modification de l'action %1"_s, "real French.lng: placeholder");
    expect(tr(fr, "Web site address: "), u"Adresse du site web : "_s, "real French.lng: trailing space");
    expect(tr(fr, "Light / dark"), u"Clair / sombre"_s, "real French.lng: theme button");
    return bad;
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

    QTemporaryDir fileDir;  // ctest may run the two tests at the same time: no shared name in the temp folder
    const QString file = fileDir.filePath(u"antrenamer-test-panel.arb"_s);
    for (int t = 0; t <= last; ++t) {
        ActionList defaults;
        defaults.add(ActionType(t));
        if (!defaults.save(file)) { std::fprintf(stderr, "FAIL type %d: cannot write %s\n", t, qPrintable(file)); ++failures; continue; }
        QFile in(file);
        if (!in.open(QIODevice::ReadOnly)) { std::fprintf(stderr, "FAIL type %d: cannot read %s\n", t, qPrintable(file)); ++failures; continue; }
        const QString xml = QString::fromUtf8(in.readAll());
        in.close();
        QFile out(file);
        if (!out.open(QIODevice::WriteOnly)) { std::fprintf(stderr, "FAIL type %d: cannot rewrite %s\n", t, qPrintable(file)); ++failures; continue; }
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
    failures += lngChecks();
    std::fprintf(stderr, failures ? "panel round trip: %d failure(s)\n" : "panel round trip: all %d action types OK\n", failures ? failures : last + 1);
    return failures ? 1 : 0;
}
