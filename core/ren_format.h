#pragma once
#include <QDateTime>
#include <QString>
#include <functional>

namespace ren {

using TagGetter = std::function<bool(const QString& tag, QString& value)>;

enum FormatOption { AllowQuotes = 1, AllowIf = 2 };

// Expands %tag% (via getter), %% , "quoted", $if(cond,then,else) and $$.
QString advancedFormat(const QString& mask, const TagGetter& get, int options = 0,
                       QChar stopAt = QChar(0));

// Delphi FormatDateTime subset: d dd ddd dddd m mm mmm mmmm yy yyyy h hh n nn s ss z zzz
// ampm, 'literal', "literal". 'm' right after h or before s means minutes, as in Delphi.
QString formatDateTime(const QString& fmt, const QDateTime& dt);

}  // namespace ren
