#include "lngtranslator.h"

#include "appsettings.h"
#include "helpwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLocale>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QSettings>
#include <QXmlStreamReader>

using namespace Qt::StringLiterals;

namespace {

LngTranslator* g_translator = nullptr;

// The .lng files are windows-1252; Latin-1 plus the 0x80-0x9F block.
QString decode1252(const QByteArray& b) {
    static const char16_t hi[32] = {0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
                                    0x2039, 0x0152, 0x8D, 0x017D, 0x8F, 0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
                                    0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178};
    QString s;
    s.reserve(b.size());
    for (uchar c : b) s += (c >= 0x80 && c < 0xA0) ? QChar(hi[c - 0x80]) : QChar(c);
    return s;
}

// Each .lng declares its own encoding (windows-1250, gb2312, ...); fall back to windows-1252.
QString decodeLng(const QByteArray& bytes) {
    static const QRegularExpression re(u"^<\\?xml[^>]*encoding=[\"']([^\"']+)[\"']"_s);
    const auto m = re.match(QString::fromLatin1(bytes.left(200)));
    if (m.hasMatch()) {
        const QString name = m.captured(1);
        if (name.compare(u"utf-8", Qt::CaseInsensitive) == 0) return QString::fromUtf8(bytes);
        QStringDecoder dec(name.toLatin1().constData());
        if (dec.isValid()) {
            const QString out = dec(bytes);
            if (!dec.hasError()) return out;
        }
    }
    return decode1252(bytes);
}

QString readLng(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QString s = decodeLng(f.readAll());
    // QString input ignores the declared encoding, but strip it anyway to be safe
    s.replace(QRegularExpression(u"^<\\?xml[^>]*\\?>"_s), QString());
    return s;
}

// Flattens the file: "MainForm/ActionAddFiles/Caption" -> value. Item elements are told apart by Name/Index.
QHash<QString, QString> parse(const QString& xml) {
    QHash<QString, QString> out;
    QXmlStreamReader r(xml);
    QStringList path;
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            QString n = r.name().toString();
            const auto attrs = r.attributes();
            if (attrs.hasAttribute(u"Name"_s)) n += u'[' + attrs.value(u"Name"_s).toString() + u']';
            else if (attrs.hasAttribute(u"Index"_s)) n += u'[' + attrs.value(u"Index"_s).toString() + u']';
            path << n;
            for (const auto& a : attrs) {
                if (a.name() == u"Name" || a.name() == u"Index") continue;
                out.insert(path.join(u'/') + u'/' + a.name().toString(), a.value().toString());
            }
        } else if (r.isEndElement() && !path.isEmpty()) {
            path.removeLast();
        }
    }
    return out;
}

// "%s", "%d" (Delphi) and "%1".."%9" (Qt) all become "%@" so both spellings find each other.
QString normalize(QString s) {
    static const QRegularExpression re(u"%(?:\\d+:)?[sd]|%[1-9]\\b"_s);
    s.replace(re, u"%@"_s);
    return s.trimmed();
}

// Turns the Delphi placeholders of a translated string into %1, %2, ... in order.
QString toQtPlaceholders(QString s) {
    static const QRegularExpression re(u"%(?:\\d+:)?[sd]"_s);
    int n = 0;
    QString out;
    qsizetype last = 0;
    auto it = re.globalMatch(s);
    while (it.hasNext()) {
        auto m = it.next();
        out += s.mid(last, m.capturedStart() - last) + u'%' + QString::number(++n);
        last = m.capturedEnd();
    }
    return out + s.mid(last);
}

}  // namespace

LngTranslator* lngTranslator() { return g_translator; }
void setLngTranslator(LngTranslator* t) { g_translator = t; }

// Data folders (Languages, help) sit next to the program (Windows zip, build folder) or, in a Linux package,
// in <prefix>/share/antrenamer-qt next to <prefix>/bin.
QString dataPath(const QString& name) {
    const QDir app(QCoreApplication::applicationDirPath());
    const QStringList candidates = {app.filePath(name), app.filePath(u"../share/antrenamer-qt/"_s + name)};
    for (const QString& c : candidates)
        if (QFileInfo::exists(c)) return QDir::cleanPath(c);
    return candidates.first();
}

QString languageDir() { return dataPath(u"Languages"_s); }

QList<LangInfo> availableLanguages(const QString& dir) {
    QList<LangInfo> list;
    for (const QFileInfo& fi : QDir(dir).entryInfoList({u"*.lng"_s}, QDir::Files, QDir::Name)) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        QByteArray raw = f.read(2500);
        raw.truncate(raw.lastIndexOf(10));  // cut at a line end so a multi-byte character is never split
        const QString head = decodeLng(raw);
        auto field = [&head](const QString& key) {
            const auto m = QRegularExpression(u"^"_s + key + u"=([^\r\n]*)"_s, QRegularExpression::MultilineOption).match(head);
            return m.hasMatch() ? m.captured(1).trimmed() : QString();
        };
        const QString local = field(u"LocalName"_s);
        list.push_back({fi.completeBaseName(), local.isEmpty() ? fi.completeBaseName() : local, field(u"Version"_s),
                        field(u"Authors"_s).replace(u'|', u", "_s), field(u"Comments"_s)});
    }
    return list;
}

QString defaultLanguage() {
    const QString saved = appSettings().value(u"language"_s).toString();
    if (!saved.isEmpty() && saved != u"?") return saved;
    const QString eng = QLocale::languageToString(QLocale::system().language());
    for (const LangInfo& l : availableLanguages(languageDir()))
        if (l.file == eng || l.file.startsWith(eng + u' ')) return l.file;
    return u"English"_s;
}

QString helpLanguageCode() {
    const QString lang = defaultLanguage();
    return lang == u"French" ? u"fr"_s : lang == u"Croatian" ? u"hr"_s : lang == u"Tamil" ? u"ta"_s : u"en"_s;
}

void openHelp(const QString& topic) { showHelp(topic, QApplication::activeWindow()); }

bool LngTranslator::loadLanguage(const QString& dir, const QString& language) {
    map_.clear();
    controls_.clear();
    language_ = language;
    if (language == u"English") return false;
    const auto en = parse(readLng(QDir(dir).filePath(u"English.lng"_s)));
    const auto tr = parse(readLng(QDir(dir).filePath(language + u".lng"_s)));
    if (en.isEmpty() || tr.isEmpty()) return false;
    // QHash order changes from one run to the next, and the same English text can be in several places (the caption and
    // the hint of a button): a fixed order makes the translation of such a text the same every time (caption first).
    QStringList keys = en.keys();
    keys.sort();
    for (const QString& enKey : keys) {
        const auto it = en.constFind(enKey);
        const auto t = tr.constFind(it.key());
        if (t == tr.constEnd() || it.value().isEmpty() || t.value().isEmpty()) continue;
        const QString key = it.key();
        auto add = [this](const QString& e, const QString& f) {
            if (e.isEmpty() || f.isEmpty() || e == f) return;
            const QString k = normalize(e);
            if (!map_.contains(k)) map_.insert(k, toQtPlaceholders(f));
        };
        if (key.endsWith(u"/Hint")) {  // "Short|Long": both halves are used on their own
            const int a = int(it.value().indexOf(u'|')), b = int(t.value().indexOf(u'|'));
            if (a >= 0 && b >= 0) {
                add(it.value().left(a), t.value().left(b));
                add(it.value().mid(a + 1), t.value().mid(b + 1));
                continue;
            }
        }
        add(it.value(), t.value());
        if (key.endsWith(u"/Caption")) {
            const QStringList parts = key.split(u'/');
            controls_.insert(parts.value(parts.size() - 2), t.value());
        }
    }
    return !map_.isEmpty();
}

QString LngTranslator::translate(const char* /*context*/, const char* sourceText, const char* /*disambiguation*/, int /*n*/) const {
    const QString src = QString::fromUtf8(sourceText);
    const auto it = map_.constFind(normalize(src));
    if (it != map_.constEnd()) return *it;
    if (language_ == u"French") {  // strings that do not exist in the original program
        static const QHash<QString, QString> extra = {
            {u"Portable mode: keep the settings in the program folder"_s,
             u"Mode portable : conserver les réglages dans le dossier du programme"_s},
            {u"Settings storage"_s, u"Stockage des réglages"_s},
            {u"Could not write the settings in the program folder."_s,
             u"Impossible d'écrire les réglages dans le dossier du programme."_s},
            {u"The new language is used the next time the program starts."_s,
             u"La nouvelle langue sera utilisée au prochain démarrage du programme."_s},
            {u"History"_s, u"Historique"_s},
            {u"Natural sort in files list (2 before 10) instead of alphabetical"_s,
             u"Tri naturel dans la liste des fichiers (2 avant 10) au lieu du tri alphabétique"_s},
            {u"Modern interface layout (used at the next start)"_s,
             u"Disposition moderne de l'interface (prise en compte au prochain démarrage)"_s},
            {u"The new interface is used the next time the program starts."_s,
             u"La nouvelle interface sera utilisée au prochain démarrage du programme."_s},
            {u"Modify"_s, u"Modifier"_s},
            {u"Pages layout (modern interface):"_s, u"Disposition des pages (interface moderne) :"_s},
            {u"Tabs"_s, u"Onglets"_s},
            {u"Single screen, in bands"_s, u"Écran unique, en bandes"_s},
            {u"Single screen, in columns"_s, u"Écran unique, en colonnes"_s},
            {u"Validate the modification"_s, u"Valider la modification"_s},
            {u"Cancel the modification"_s, u"Annuler la modification"_s},
            {u"Modification of action %1"_s, u"Modification de l'action %1"_s},
            {u"Replace the selected action with the current settings (double-click an action to load its settings)"_s,
             u"Remplacer l'action sélectionnée par les réglages actuels (double-cliquez sur une action pour charger ses réglages)"_s},
            {u"Settings loaded: change them, then validate the modification"_s,
             u"Réglages chargés : modifiez-les, puis validez la modification"_s},
            {u"Windows Explorer"_s, u"Explorateur Windows"_s},
            {u"Add to Ant Renamer Qt"_s, u"Ajouter à Ant Renamer Qt"_s},
            {u"Add \"Add to Ant Renamer Qt\" to the context menu of files and folders"_s,
             u"Ajouter « Ajouter à Ant Renamer Qt » au menu contextuel des fichiers et dossiers"_s},
            {u"On Windows 11 the entry is in \"Show more options\" (or Shift+F10)."_s,
             u"Sous Windows 11, l'entrée se trouve dans « Afficher plus d'options » (ou Maj+F10)."_s},
            {u"Could not change the Explorer context menu."_s, u"Impossible de modifier le menu contextuel de l'Explorateur."_s},
            {u"seconds"_s, u"secondes"_s},
            {u"Position:"_s, u"Position :"_s},
            {u"String:"_s, u"Chaîne :"_s},
            {u"License:"_s, u"Licence :"_s},
            {u"Web site address: "_s, u"Adresse du site web : "_s},
            {u"E-mail address: "_s, u"Adresse e-mail : "_s},
            {u"Ant Renamer Qt is a C++/Qt port of Ant Renamer by Antoine Potten, developed in parallel with the original program. Port (Windows and Linux): Frédéric Biron."_s,
             u"Ant Renamer Qt est un portage C++/Qt d'Ant Renamer d'Antoine Potten, développé en parallèle du programme d'origine. Portage (Windows et Linux) : Frédéric Biron."_s},
            {u"Ant Renamer Qt support: "_s, u"Assistance pour Ant Renamer Qt : "_s},
            {u"issues and discussions"_s, u"issues et discussions"_s},
            {u"Port:"_s, u"Portage :"_s},
            {u"C++/Qt 6 port and Linux port: "_s, u"Portage C++/Qt 6 et portage Linux : "_s},
            {u"Components:"_s, u"Composants :"_s},
            {u"Translations:"_s, u"Traductions :"_s},
            {u"Thanks to:"_s, u"Remerciements :"_s},
            {u"...and all the people that sent me suggestions and comments"_s,
             u"...et toutes les personnes qui m'ont envoyé des suggestions et des commentaires"_s},
            {u"This program is free software; you can redistribute it and/or modify it under the terms of the GNU General "
             "Public License as published by the Free Software Foundation; either version 2 of the License, or (at your "
             "option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY "
             "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
             "<a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>GNU General Public License</a> for more details."_s,
             u"Ce programme est un logiciel libre ; vous pouvez le redistribuer et/ou le modifier selon les termes de la "
             "Licence Publique Générale GNU telle que publiée par la Free Software Foundation ; soit la version 2 de la "
             "licence, soit (à votre convenance) toute version ultérieure. Ce programme est distribué dans l'espoir qu'il "
             "sera utile, mais SANS AUCUNE GARANTIE, sans même la garantie implicite de COMMERCIALISATION ou "
             "d'ADÉQUATION À UN USAGE PARTICULIER. Consultez la <a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>"
             "Licence Publique Générale GNU</a> pour plus de détails."_s},
        };
        if (const auto e = extra.constFind(src); e != extra.constEnd()) return *e;
    }
    if (qEnvironmentVariableIsSet("ANTREN_MISSING")) {  // developer aid: list strings without a translation
        std::fprintf(stderr, "MISSING: %s\n", sourceText);
    }
    return {};
}

QString LngTranslator::control(const QString& name, const QString& fallback) const {
    return controls_.value(name, fallback);
}
