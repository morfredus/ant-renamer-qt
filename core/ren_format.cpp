// Port of functions_strformat.pas (AdvancedFormat) + a Delphi-compatible FormatDateTime.
#include "ren_format.h"

#include <QLocale>

using namespace Qt::StringLiterals;

namespace ren {

namespace {

const QString kIf = QStringLiteral("$if(");

bool startsAt(const QString& s, int i, const QString& what) { return s.mid(i, what.size()) == what; }

// Index of the next `toFind` that is outside quotes and outside $if( ... ) groups.
int findNextChar(const QString& mask, int i, QChar toFind, int opts) {
    bool inStr = false;
    int inCond = 0;
    const int L = mask.size();
    while (i < L) {
        const QChar c = mask[i];
        if (c == toFind && !(inStr || inCond > 0)) break;
        if (c == u')' && !inStr) {
            if (inCond > 0) --inCond;
            ++i;
        } else if ((opts & AllowIf) && c == u'$' && !inStr) {
            if (i + 1 < L && mask[i + 1] == u'$') i += 2;
            else if (startsAt(mask, i, kIf)) { ++inCond; i += kIf.size(); }
            else ++i;  // lone '$': the Delphi original looped forever here
        } else if ((opts & AllowQuotes) && c == u'"') {
            if (i + 1 < L && mask[i + 1] == u'"') i += 2;
            else { inStr = !inStr; ++i; }
        } else {
            ++i;
        }
    }
    return i;
}

}  // namespace

QString advancedFormat(const QString& mask, const TagGetter& get, int options, QChar stopAt) {
    QString result;
    bool inStr = false;
    int i = 0;
    const int L = mask.size();
    while (i < L) {
        const QChar c = mask[i];
        if (c == stopAt && !inStr) return result;
        if ((options & AllowQuotes) && c == u'"') {
            if (!(i + 1 < L && mask[i + 1] == u'"')) { inStr = !inStr; ++i; }
            else { result += u'"'; i += 2; }
        } else if ((options & AllowIf) && c == u'$' && !inStr) {
            if (i + 1 < L && mask[i + 1] == u'$') { result += u'$'; i += 2; }
            else if (startsAt(mask, i, kIf)) {
                i += kIf.size();
                if (!advancedFormat(mask.mid(i), get, options, u',').isEmpty()) {
                    i = findNextChar(mask, i, u',', options) + 1;
                    result += advancedFormat(mask.mid(i), get, options, u',');
                    i = findNextChar(mask, i, u')', options) + 1;
                } else {
                    i = findNextChar(mask, i, u',', options) + 1;
                    i = findNextChar(mask, i, u',', options) + 1;
                    result += advancedFormat(mask.mid(i), get, options, u')');
                    i = findNextChar(mask, i, u')', options) + 1;
                }
            } else {
                ++i;
            }
        } else if (c == u'%' && i < L - 1 && !inStr) {
            if (mask[i + 1] == u'%') { result += u'%'; i += 2; continue; }
            int j = findNextChar(mask, i + 1, u'%', options);
            QString val;
            if (get(mask.mid(i + 1, j - i - 1), val)) { result += val; i = j + 1; }
            else { result += u'%'; ++i; }
        } else {
            result += c;
            ++i;
        }
    }
    return result;
}

QString formatDateTime(const QString& f, const QDateTime& dt) {
    const QDate d = dt.date();
    const QTime t = dt.time();
    const QLocale loc;
    QString out;
    bool lastWasHour = false;
    const int L = f.size();
    auto run = [&](int i) { int n = 1; while (i + n < L && f[i + n].toLower() == f[i].toLower()) ++n; return n; };
    auto p2 = [](int v) { return QStringLiteral("%1").arg(v, 2, 10, QLatin1Char('0')); };
    for (int i = 0; i < L;) {
        const QChar c = f[i].toLower();
        if (c == u'\'' || c == u'"') {
            int e = f.indexOf(f[i], i + 1);
            if (e < 0) e = L;
            out += f.mid(i + 1, e - i - 1);
            i = e + 1;
            continue;
        }
        const int n = run(i);
        QString piece;
        bool hour = false;
        switch (c.unicode()) {
        case 'd': piece = n == 1 ? QString::number(d.day()) : n == 2 ? p2(d.day())
                        : n == 3 ? loc.dayName(d.dayOfWeek(), QLocale::ShortFormat)
                                 : loc.dayName(d.dayOfWeek(), QLocale::LongFormat); break;
        case 'y': piece = n <= 2 ? p2(d.year() % 100) : QStringLiteral("%1").arg(d.year(), 4, 10, QLatin1Char('0')); break;
        case 'h': piece = n == 1 ? QString::number(t.hour()) : p2(t.hour()); hour = true; break;
        case 'n': piece = n == 1 ? QString::number(t.minute()) : p2(t.minute()); break;
        case 's': piece = n == 1 ? QString::number(t.second()) : p2(t.second()); break;
        case 'z': piece = n == 1 ? QString::number(t.msec()) : QStringLiteral("%1").arg(t.msec(), 3, 10, QLatin1Char('0')); break;
        case 'a': if (f.mid(i, 4).toLower() == u"ampm") { piece = t.hour() < 12 ? u"AM"_s : u"PM"_s; out += piece; i += 4; lastWasHour = false; continue; }
                  piece = QString(1, f[i]); break;
        case 'm': {
            int k = i + n;  // minutes if it follows an hour or precedes seconds
            while (k < L && !f[k].isLetter()) ++k;
            if (lastWasHour || (k < L && f[k].toLower() == u's'))
                piece = n == 1 ? QString::number(t.minute()) : p2(t.minute());
            else
                piece = n == 1 ? QString::number(d.month()) : n == 2 ? p2(d.month())
                        : n == 3 ? loc.monthName(d.month(), QLocale::ShortFormat)
                                 : loc.monthName(d.month(), QLocale::LongFormat);
            break;
        }
        default: out += f.mid(i, n); i += n; if (!c.isSpace() && c != u':') lastWasHour = false; continue;
        }
        out += piece;
        lastWasHour = hour;
        i += n;
    }
    return out;
}

}  // namespace ren
