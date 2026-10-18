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
#include <cstdio>

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
QString decodeLng(QByteArray bytes, const QString& fileName = {}) {
    if (bytes.startsWith("\xEF\xBB\xBF")) bytes.remove(0, 3);  // an editor may add a UTF-8 BOM: it is not part of the XML
    static const QRegularExpression re(u"^<\\?xml[^>]*encoding=[\"']([^\"']+)[\"']"_s);
    const auto m = re.match(QString::fromLatin1(bytes.left(200)));
    if (m.hasMatch()) {
        const QString name = m.captured(1);
        if (name.compare(u"utf-8", Qt::CaseInsensitive) == 0) {
            QStringDecoder utf8(QStringDecoder::Utf8);
            const QString out = utf8(bytes);
            if (!utf8.hasError()) return out;
            if (!fileName.isEmpty()) std::fprintf(stderr, "Language file %s: declared UTF-8 but it is not valid UTF-8, read as windows-1252\n", qPrintable(fileName));
            return decode1252(bytes);
        }
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
    QString s = decodeLng(f.readAll(), QFileInfo(path).fileName());
    // QString input ignores the declared encoding, but strip it anyway to be safe
    s.replace(QRegularExpression(u"^<\\?xml[^>]*\\?>"_s), QString());
    return s;
}

// Flattens the file: "MainForm/ActionAddFiles/Caption" -> value. Item elements are told apart by Name/Index.
QHash<QString, QString> parse(const QString& xml, const QString& fileName = {}) {
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
    // What was read before the error is kept (a damaged file still translates what it can), but it must not go unnoticed
    if (r.hasError() && !fileName.isEmpty())
        std::fprintf(stderr, "Language file %s: XML error at line %lld: %s (%zu texts read before it)\n", qPrintable(fileName),
                     static_cast<long long>(r.lineNumber()), qPrintable(r.errorString()), size_t(out.size()));
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
    const auto en = parse(readLng(QDir(dir).filePath(u"English.lng"_s)), u"English.lng"_s);
    const auto tr = parse(readLng(QDir(dir).filePath(language + u".lng"_s)), language + u".lng"_s);
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
        // Pieces of text are glued by the code (" at position %1", " using creation date"): the English text decides
        // whether the translation starts or ends with spaces, whatever the translator typed.
        auto edge = [](const QString& s, bool leading) {
            qsizetype n = 0;
            while (n < s.size() && s[leading ? n : s.size() - 1 - n].isSpace()) ++n;
            return leading ? s.left(n) : s.right(n);
        };
        auto add = [this, &edge](const QString& e, const QString& f) {
            if (e.isEmpty() || f.isEmpty() || e == f) return;
            const QString k = normalize(e);
            if (!map_.contains(k)) map_.insert(k, toQtPlaceholders(edge(e, true) + f.trimmed() + edge(e, false)));
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
    if (qEnvironmentVariableIsSet("ANTREN_MISSING")) {  // developer aid: list strings without a translation
        std::fprintf(stderr, "MISSING: %s\n", sourceText);
    }
    return {};
}

QString LngTranslator::control(const QString& name, const QString& fallback) const {
    return controls_.value(name, fallback);
}
