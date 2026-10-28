#include "actionpanel.h"

#include "appsettings.h"
#include "lngtranslator.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QCompleter>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QXmlStreamReader>

using namespace ren;
using namespace Qt::StringLiterals;

// --- HistoryCombo -----------------------------------------------------------------------------
HistoryCombo::HistoryCombo() {
    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void HistoryCombo::remember() {
    const QString t = currentText();
    if (t.isEmpty()) {
        return;
    }
    const int i = findText(t, Qt::MatchExactly);
    if (i >= 0) {
        removeItem(i);
    }
    insertItem(0, t);
    setCurrentIndex(0);
    while (count() > qMax(1, appOptions().historyMax)) {
        removeItem(count() - 1);
    }
}

// --- widgets read by makeAction() ---------------------------------------------------------------
struct ActionPanel::W {
    HistoryCombo *extNew, *srcSearch, *srcRepl, *insStr, *delStr, *enumMask, *mp3Mask, *dtMask, *rndMask, *caseAfter, *reExpr,
        *reRepl, *exifMask;
    QCheckBox *extNoRepl, *srAll, *srCase, *srExt, *srOnlyExt, *srWild, *msExt, *msCase, *insExt, *delExt, *enumRestart,
        *enumShare, *exShare, *mp3Two, *dtSuffix, *caseLocale, *caseIncl, *caseOnly, *flAppend, *flOnlyExt, *exRestart;
    QSpinBox *insPos, *mvFrom, *mvCount, *mvTo, *delNb, *delPos, *enumStart, *enumDigits, *enumIncr, *dtOffset, *exifOffset,
        *exStart, *exDigits, *exIncr;
    std::vector<QRadioButton*> insDir, mvFrom_, mvTo_, delWhere, delDir, dtWhich, rndMethod, caseType;
};

static const char* const kNames[] = {
    QT_TRANSLATE_NOOP("Ren", "Change extension"),
    QT_TRANSLATE_NOOP("Ren", "String replacement"),
    QT_TRANSLATE_NOOP("Ren", "Multiple string replacement"),
    QT_TRANSLATE_NOOP("Ren", "String insertion"),
    QT_TRANSLATE_NOOP("Ren", "Move string"),
    QT_TRANSLATE_NOOP("Ren", "Characters deletion"),
    QT_TRANSLATE_NOOP("Ren", "Enumeration"),
    QT_TRANSLATE_NOOP("Ren", "Use mp3 tag info"),
    QT_TRANSLATE_NOOP("Ren", "Use date & time"),
    QT_TRANSLATE_NOOP("Ren", "Random names"),
    QT_TRANSLATE_NOOP("Ren", "Change case"),
    QT_TRANSLATE_NOOP("Ren", "Take names from list"),
    QT_TRANSLATE_NOOP("Ren", "Regular expression"),
    QT_TRANSLATE_NOOP("Ren", "Use EXIF info"),
};

ActionPanel::ActionPanel(QWidget* parent) : QWidget(parent), w_(std::make_unique<W>()) {
    auto* split = new QSplitter(Qt::Horizontal);
    split->setChildrenCollapsible(false);
    list_ = new QListWidget;
    list_->setToolTip(tr("Actions list"));
    for (const char* n : kNames) {
        auto* it = new QListWidgetItem(QCoreApplication::translate("Ren", n), list_);
        it->setSizeHint(QSize(0, 20));
    }
    pages_ = new QStackedWidget;
    split->addWidget(list_);
    split->addWidget(pages_);
    split->setStretchFactor(1, 1);
    split->setSizes({200, 500});
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(split);

    buildPages();
    connect(list_, &QListWidget::currentRowChanged, this, [this](int r) {
        pages_->setCurrentIndex(r + 1);  // page 0 is the empty one
        emit changed();
    });
}

ActionPanel::~ActionPanel() = default;

int ActionPanel::currentIndex() const {
    return list_->currentRow();
}
void ActionPanel::selectAction(int i) {
    list_->setCurrentRow(i);
}

// --- page building helpers -----------------------------------------------------------------------
ActionPanel::Page ActionPanel::newPage(const QString& name) {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(8, 8, 8, 8);
    auto* scroll = new QScrollArea;  // long pages (EXIF with its counter) stay reachable in a small window
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(w);
    pages_->addWidget(scroll);
    return {w, v, name};
}

HistoryCombo* ActionPanel::combo(Page& p, const QString& key, const QString& label, const QString& def, bool place) {
    auto* c = new HistoryCombo;
    c->setCurrentText(def);
    if (place) {
        p.v->addWidget(new QLabel(label));
        p.v->addWidget(c);
    }
    combos_.push_back(c);
    connect(c, &QComboBox::editTextChanged, this, &ActionPanel::changed);
    fields_.push_back(
        {p.name + u'/' + key, [c] { return c->currentText(); }, [c](const QVariant& v) { c->setCurrentText(v.toString()); }});
    return c;
}

QSpinBox* ActionPanel::spin(Page& p, const QString& key, const QString& label, int def, int min, int max, const QString& suffix) {
    auto* s = new QSpinBox;
    s->setRange(min, max);
    s->setValue(def);
    auto* row = new QHBoxLayout;
    row->addWidget(new QLabel(label));
    row->addWidget(s);
    if (!suffix.isEmpty()) {
        row->addWidget(new QLabel(suffix));
    }
    row->addStretch();
    p.v->addLayout(row);
    connect(s, &QSpinBox::valueChanged, this, &ActionPanel::changed);
    fields_.push_back({p.name + u'/' + key, [s] { return s->value(); }, [s](const QVariant& v) { s->setValue(v.toInt()); }});
    return s;
}

QCheckBox* ActionPanel::check(Page& p, const QString& key, const QString& label, bool def) {
    // A QCheckBox never wraps its text: use a bare box plus a wrapping label that toggles it when clicked.
    struct Toggle : QObject {
        QCheckBox* box;
        bool eventFilter(QObject* o, QEvent* e) override {
            if (e->type() != QEvent::MouseButtonRelease) {
                return false;
            }
            const auto* me = static_cast<QMouseEvent*>(e);  // the left button, released over the label
            if (me->button() != Qt::LeftButton || !static_cast<QWidget*>(o)->rect().contains(me->position().toPoint())) {
                return false;
            }
            box->toggle();
            return true;
        }
    };
    auto* c = new QCheckBox;
    c->setChecked(def);
    auto* text = new QLabel(label);
    text->setWordWrap(true);
    text->setMinimumWidth(1);  // a wrapping label must not force the page wider than the panel
    auto* toggle = new Toggle;
    toggle->setParent(text);
    toggle->box = c;
    text->installEventFilter(toggle);
    auto* row = new QHBoxLayout;
    row->setSpacing(2);
    row->addWidget(c, 0, Qt::AlignTop);
    row->addWidget(text, 1);
    p.v->addLayout(row);
    connect(c, &QCheckBox::toggled, this, &ActionPanel::changed);
    fields_.push_back(
        {p.name + u'/' + key, [c] { return c->isChecked(); }, [c](const QVariant& v) { c->setChecked(v.toBool()); }});
    return c;
}

std::vector<QRadioButton*> ActionPanel::radios(Page& p, const QString& key, const QStringList& labels, int def, bool vertical,
                                               QWidget* firstExtra) {
    std::vector<QRadioButton*> r;
    auto* group = new QButtonGroup(this);  // one exclusive group per call, not per page
    QBoxLayout* row = vertical ? static_cast<QBoxLayout*>(new QVBoxLayout) : static_cast<QBoxLayout*>(new QHBoxLayout);
    for (const QString& t : labels) {
        auto* b = new QRadioButton(t);
        group->addButton(b);
        r.push_back(b);
        if (r.size() == 1 && firstExtra) {
            auto* h = new QHBoxLayout;
            h->addWidget(b);
            h->addWidget(firstExtra, 1);
            row->addLayout(h);
        } else {
            row->addWidget(b);
        }
        connect(b, &QRadioButton::toggled, this, &ActionPanel::changed);
    }
    if (!vertical) {
        row->addStretch();
    }
    p.v->addLayout(row);
    r[size_t(def)]->setChecked(true);
    fields_.push_back({p.name + u'/' + key,
                       [r] {
                           for (size_t i = 0; i < r.size(); ++i) {
                               if (r[i]->isChecked()) {
                                   return QVariant(int(i));
                               }
                           }
                           return QVariant(0);
                       },
                       [r](const QVariant& v) { r[size_t(qBound(0, v.toInt(), int(r.size()) - 1))]->setChecked(true); }});
    return r;
}

void ActionPanel::notes(Page& p, const QString& control, const QString& english) {
    QString html = lngTranslator() ? lngTranslator()->control(control, english) : english;
    if (control == u"lblExifNotes") {  // %num% is new in this version: reuse the word the Enumeration note uses
        QString number = u"Number"_s;
        if (lngTranslator()) {
            static const QRegularExpression re(u"%num% = ([^<]*)"_s);
            const auto m = re.match(lngTranslator()->control(u"lblEnumNotes"_s, QString()));
            if (m.hasMatch()) {
                number = m.captured(1).trimmed();
            }
        }
        html = html.trimmed();
        const QString extra = u"<BR>- %num% = "_s + number + u"<BR>- "_s +
                              tr("%IptcKeywords%, %XmpTitle%... = IPTC and XMP data (keywords, title, author, place...)");
        const qsizetype at = html.indexOf(u"<BR>- %% ="_s);  // keep the "see Help" line last
        if (at >= 0) {
            html.insert(at, extra);
        } else {
            html += extra;
        }
    }
    static const QRegularExpression link(u"<link>(.*?)</link>"_s, QRegularExpression::CaseInsensitiveOption);
    QString h = html;
    h.replace(link, u"<a href=\"\\1\">\\1</a>"_s);
    auto* l = new QLabel(h);
    l->setWordWrap(true);
    l->setMinimumWidth(1);  // a wrapping label must not force the page wider than the panel
    l->setTextFormat(Qt::RichText);
    l->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    connect(l, &QLabel::linkActivated, this, [this, control](const QString& href) {
        for (int i = 0; i < list_->count(); ++i) {
            if (list_->item(i)->text().compare(href, Qt::CaseInsensitive) == 0) {
                list_->setCurrentRow(i);
                return;
            }
        }
        emit helpRequested(control == u"lblRegexpNotes" ? u"regexp"_s
                           : control == u"lblExifNotes" ? u"metadata_tags"_s
                                                        : u"format"_s);  // any other link is the \"Help\" link
    });
    p.v->addSpacing(8);
    p.v->addWidget(l);
}

// --- the 14 pages --------------------------------------------------------------------------------
void ActionPanel::buildPages() {
    newPage(QStringLiteral("empty"));
    W& w = *w_;
    const QString tagNotes = tr("- %name% = Original file name (without extension)<BR>"
                                "- %ext% = Original extension (including the dot)<BR>");
    const QString folderNote = tr("- %folderN% = Folder name, where N is the level (1 = current, 2 = parent, etc.)<BR>"
                                  "- %% = % character<BR>see <LINK>Help</LINK> for more information");
    const QString notesHead = tr("Notes:") + u"<BR>"_s;

    {  // 0 Change extension
        Page p = newPage(u"ChangeExt"_s);
        w.extNew = combo(p, u"NewExt"_s, tr("Replace extension by:"), u"ext"_s);
        w.extNoRepl = check(p, u"NoReplace"_s, tr("Do not replace extension, append it to current full name"), false);
        notes(p, u"lblChangeExtNotes"_s,
              notesHead + tr("- Do not include the dot, just enter the new extension<br>"
                             "- If you want to make an incremented extension, use the <LINK>Enumeration</LINK> action"));
        p.v->addStretch();
    }
    {  // 1 String replacement
        Page p = newPage(u"StrRepl"_s);
        w.srcSearch = combo(p, u"Search"_s, tr("Search for:"), u"old"_s);
        w.srcRepl = combo(p, u"Repl"_s, tr("Replace by:"), u"new"_s);
        w.srAll = check(p, u"All"_s, tr("Replace all occurences"), true);
        w.srCase = check(p, u"Case"_s, tr("Case sensitive"), true);
        w.srExt = check(p, u"Ext"_s, tr("Include extension in search"), false);
        w.srOnlyExt = check(p, u"OnlyExt"_s, tr("Apply to extension instead of name"), false);
        w.srWild = check(p, u"Wildcards"_s, tr("Use wildcards (* = any text, ? = any character)"), false);
        p.v->addStretch();
    }
    {  // 2 Multiple string replacement
        Page p = newPage(u"MultstrRepl"_s);
        buildMultstr(p);
    }
    {  // 3 String insertion
        Page p = newPage(u"StrIns"_s);
        w.insStr = combo(p, u"Str"_s, tr("String to insert:"), u"new"_s);
        w.insPos = spin(p, u"Pos"_s, tr("At position:"), 0, 0, 1000);
        w.insDir = radios(p, u"Dir"_s, {tr("From begin"), tr("From end")}, 0);
        w.insExt = check(p, u"Ext"_s, tr("Apply to extension instead of name"), false);
        notes(p, u"lblStrInsNotes"_s,
              notesHead +
                  tr("- If you want to insert a string after or before another one, use the <LINK>String replacement</LINK> "
                     "action<br>") +
                  tagNotes + folderNote);
        p.v->addStretch();
    }
    {  // 4 Move string
        Page p = newPage(u"MoveStr"_s);
        w.mvFrom = spin(p, u"FromPos"_s, tr("From position:"), 0, 0, 1000);
        w.mvFrom_ = radios(p, u"FromDir"_s, {tr("From begin"), tr("From end")}, 0);
        w.mvCount = spin(p, u"Count"_s, tr("Number of characters to move:"), 1, 1, 1000);
        w.mvTo = spin(p, u"ToPos"_s, tr("To position:"), 0, 0, 1000);
        w.mvTo_ = radios(p, u"ToDir"_s, {tr("From begin"), tr("From end")}, 0);
        notes(p, u"lblMoveStrNotes"_s,
              notesHead + tr("- If you want to move a specified string but that is not always at the same position, use the "
                             "<link>String replacement</link> to delete it, and then the <link>String insertion</link> to insert "
                             "it at the new place<BR>"
                             "- The \"To position\" value does not include the moved characters (i.e. it is like of they have "
                             "already been deleted)"));
        p.v->addStretch();
    }
    {  // 5 Characters deletion
        Page p = newPage(u"CharDel"_s);
        w.delNb = spin(p, u"Nb"_s, tr("Number of characters to delete:"), 1, 1, 1000);
        w.delWhere = radios(p, u"Where"_s, {tr("From position:"), tr("After the string:"), tr("Before the string:")}, 0);
        w.delPos = spin(p, u"Pos"_s, tr("Position:"), 0, 0, 1000);
        w.delDir = radios(p, u"Dir"_s, {tr("From begin"), tr("From end")}, 0);
        w.delStr = combo(p, u"Str"_s, tr("String:"), u"str"_s);
        w.delExt = check(p, u"Ext"_s, tr("Apply to extension instead of name"), false);
        notes(p, u"lblCharDelNotes"_s,
              notesHead + tr("- To delete a specified string, use the <LINK>String replacement</LINK> action and replace the "
                             "string by an empty one"));
        p.v->addStretch();
    }
    {  // 6 Enumeration
        Page p = newPage(u"Enum"_s);
        w.enumMask = combo(p, u"Mask"_s, tr("Mask:"), u"File %num%%ext%"_s);
        w.enumStart = spin(p, u"Start"_s, tr("Start at:"), 1, -999999999, 999999999);
        w.enumDigits = spin(p, u"Digits"_s, tr("Number of digits:"), 3, 1, 20);
        w.enumIncr = spin(p, u"Incr"_s, tr("Increment by:"), 1, -999999, 999999);
        w.enumRestart = check(p, u"Restart"_s, tr("Go back to start value when changing folder"), false);
        w.enumShare =
            check(p, u"SharePairs"_s,
                  tr("Same number for files with the same name and different extensions (e.g. JPG + RAW + XMP)"), false);
        notes(p, u"lblEnumNotes"_s, notesHead + tagNotes + tr("- %num% = Number<BR>") + folderNote);
        p.v->addStretch();
    }
    {  // 7 MP3
        Page p = newPage(u"MP3"_s);
        w.mp3Mask = combo(p, u"Mask"_s, tr("Mask:"), u"%author% - %title%%ext%"_s);
        w.mp3Two = check(p, u"TwoDigit"_s, tr("Two-digit track number (insert a '0' in front of the number if required)"), false);
        notes(p, u"lblMP3Notes"_s,
              notesHead +
                  tr("- %author% = Author<BR>- %title% = Title<BR>- %album% = Album<BR>- %year% = Year<BR>"
                     "- %genre% = Genre<BR>- %comm% = Comment<BR>- %track% = Track number (ID3V1.1)<BR>") +
                  tr("- %name% = Original filename (without extension)<BR>- %ext% = Original extension (including dot)<BR>") +
                  tr("- %folderN% = Folder name, where N is the level (1 = current, 2 = parent, etc.)<BR>- %% = % character<BR>"
                     "- $if(condition,true-value,false-value)<BR>see <LINK>Help</LINK> for more information"));
        p.v->addStretch();
    }
    {  // 8 Date & time
        Page p = newPage(u"DateTime"_s);
        w.dtMask = combo(p, u"Mask"_s, tr("Mask:"), u"yyyy'-'mm'-'dde"_s);
        p.v->addWidget(new QLabel(tr("Date to use:")));
        w.dtWhich = radios(p, u"Which"_s, {tr("Creation"), tr("Last modification")}, 1);
        w.dtOffset = spin(p, u"Offset"_s, tr("Offset:"), 0, -999999999, 999999999, tr("seconds"));
        w.dtSuffix = check(p, u"Suffix"_s, tr("Add suffix if more than one file with the same date/time"), false);
        p.v->addWidget(new QLabel(tr("Notes:")));
        auto* memo = new QPlainTextEdit(
            tr("f\tOriginal filename (without extension)\ne\tOriginal extension (including dot)\n"
               "c\tDate and time using the standard format in Windows\nd\tDay without a leading zero (1-31)\n"
               "dd\tDay with a leading zero (01-31)\nddd\tDay, short name (Sun-Sat)\ndddd\tDay, full name (Sunday-Saturday)\n"
               "ddddd\tDate using Windows' short date format\ndddddd\tDate using Windows' long date format\n"
               "m\tMonth without a leading zero (1-12)\nmm\tMonth with a leading zero (01-12)\n"
               "mmm\tMonth, short name (Jan-Dec)\nmmmm\tMonth, full name (January-December)\n"
               "yy\tYear, 2-digit number (00-99)\nyyyy\tYear, 4-digit number (0000-9999)\n"
               "h\tHour without a leading zero (0-23)\nhh\tHour with a leading zero (00-23)\n"
               "n\tMinute without a leading zero (0-59)\nnn\tMinute with a leading zero (00-59)\n"
               "s\tSecond without a leading zero (0-59)\nss\tSecond with a leading zero (00-59)\n"
               "z\tMillisecond without a leading zero (0-999)\nzzz\tMillisecond with a leading zero (000-999)\n"
               "am/pm\tUses the 12-hour clock for the preceding h or hh\na/p\tUses the 12-hour clock for the preceding h or hh\n"
               "\"xx\"\tCharacters enclosed in quotes remain unchanged\n"
               "If m or mm immediately follows h or hh, minute rather than month is used"));
        memo->setReadOnly(true);
        p.v->addWidget(memo, 1);
    }
    {  // 9 Random
        Page p = newPage(u"Random"_s);
        w.rndMask = combo(p, u"Mask"_s, tr("Mask:"), u"%random%%ext%"_s);
        p.v->addWidget(new QLabel(tr("Method:")));
        w.rndMethod = radios(p, u"Method"_s, {tr("Random number (8 digits)"), tr("Windows tick count"), tr("GUID")}, 0);
        notes(p, u"lblRandomNotes"_s,
              notesHead + tagNotes +
                  tr("- %random% = Random generated string<BR>- %% = % character<BR>see <LINK>Help</LINK> for more information"));
        p.v->addStretch();
    }
    {  // 10 Change case
        Page p = newPage(u"Case"_s);
        w.caseAfter = combo(p, u"After"_s, QString(), u"- .+("_s, false);
        w.caseType = radios(p, u"Type"_s,
                            {tr("Upper first letter following any of these characters:"),
                             tr("Upper the first letter of the filename"), tr("Upper"), tr("Lower")},
                            0, true, w.caseAfter);
        w.caseLocale = check(p, u"Locale"_s, tr("Use Windows' locale settings to change accentuated characters"), true);
        w.caseIncl = check(p, u"IncludeExt"_s, tr("Also apply to extension"), false);
        w.caseOnly = check(p, u"OnlyExt"_s, tr("Apply to extension instead of name"), false);
        p.v->addStretch();
    }
    {  // 11 From list
        Page p = newPage(u"FromList"_s);
        buildFromList(p);
    }
    {  // 12 Regexp
        Page p = newPage(u"Regexp"_s);
        w.reExpr = combo(p, u"Expr"_s, tr("Expression:"), QString());
        w.reRepl = combo(p, u"Repl"_s, tr("New name:"), QString());
        notes(p, u"lblRegexpNotes"_s,
              notesHead + tr("- \\ character can be used to escape special characters (like $ and others), use a \\\\ for a "
                             "simple \\ in new name "
                             "(if you want to target the new name to another folder)<BR>"
                             "- Use ^ to indicate the beginning of the name and $ for the end of name<BR>"
                             "- The new name must contain $1, $2, etc. to specify where found characters will come<BR>"
                             "see <LINK>Help</LINK> for more information on regular expressions"));
        p.v->addStretch();
    }
    {  // 13 EXIF
        Page p = newPage(u"Exif"_s);
        w.exifMask = combo(p, u"Mask"_s, tr("Mask:"), u"%datetime%%ext%"_s);
        w.exifOffset = spin(p, u"Offset"_s, tr("Offset:"), 0, -999999999, 999999999, tr("seconds"));
        w.exStart = spin(p, u"NumStart"_s, tr("Start at:"), 1, -999999999, 999999999);
        w.exDigits = spin(p, u"NumDigits"_s, tr("Number of digits:"), 3, 1, 20);
        w.exIncr = spin(p, u"NumIncr"_s, tr("Increment by:"), 1, -999999, 999999);
        w.exRestart = check(p, u"NumRestart"_s, tr("Go back to start value when changing folder"), false);
        w.exShare = check(p, u"NumSharePairs"_s,
                          tr("Same number for files with the same name and different extensions (e.g. JPG + RAW + XMP)"), false);
        notes(p, u"lblExifNotes"_s,
              notesHead + tagNotes +
                  tr("- %datetimeoriginal% or %datetime% = Original date and time of the picture<BR>"
                     "- %orientation% = Picture orientation<BR>- %flash% = Flash was on or off<BR>"
                     "- %% = % character<BR>see <LINK>Help</LINK> for more information and for the complete list of EXIF, IPTC "
                     "and XMP tags"));
        p.v->addStretch();
    }
}

// --- multiple string replacement ----------------------------------------------------------------------
void ActionPanel::buildMultstr(Page& p) {
    W& w = *w_;
    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Predefined sets:")));
    setName_ = new QComboBox;
    setName_->setEditable(true);
    setName_->setInsertPolicy(QComboBox::NoInsert);
    setName_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    top->addWidget(setName_, 1);
    p.v->addLayout(top);

    table_ = new QTableWidget(1, 2);
    table_->setHorizontalHeaderLabels({tr("Search for:"), tr("Replace by:")});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->verticalHeader()->setVisible(false);
    p.v->addWidget(table_, 1);
    connect(table_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem*) {
        // always keep one empty row at the end
        const int last = table_->rowCount() - 1;
        const auto* a = table_->item(last, 0);
        const auto* b = table_->item(last, 1);
        if ((a && !a->text().isEmpty()) || (b && !b->text().isEmpty())) {
            table_->setRowCount(last + 2);
        }
        emit changed();
    });
    // Ctrl+Plus / Ctrl+Minus insert / delete a line, as in the original
    table_->installEventFilter(this);

    auto* btns = new QHBoxLayout;
    auto add = [&](const QString& t, auto fn) {
        auto* b = new QPushButton(t);
        connect(b, &QPushButton::clicked, this, fn);
        btns->addWidget(b);
    };
    add(tr("Clear Table"), [this] {
        setTableText({});
        emit changed();
    });
    add(tr("Save"), [this] {
        const QString n =
            setName_->currentText().trimmed().replace(u'/', u'-').replace(u'\\', u'-');  // "/" would open a settings sub-group
        if (n.isEmpty()) {
            return;
        }
        appSettings().setValue(u"multstr/sets/"_s + n, tableText());
        multstrSets();
        setName_->setCurrentText(n);
    });
    add(tr("Delete"), [this] {
        const QString n = setName_->currentText().trimmed().replace(u'/', u'-').replace(u'\\', u'-');
        if (n.isEmpty()) {
            return;  // an empty name would remove "multstr/sets/" itself: every saved set
        }
        appSettings().remove(u"multstr/sets/"_s + n);
        multstrSets();
    });
    add(tr("Copy/Export"), [this] { QApplication::clipboard()->setText(tableText()); });
    add(tr("Paste/Import"), [this] {
        setTableText(QApplication::clipboard()->text());
        emit changed();
    });
    btns->addStretch();
    p.v->addLayout(btns);

    w.msExt = check(p, u"IncludeExt"_s, tr("Include extension in search"), false);
    w.msCase = check(p, u"Case"_s, tr("Case sensitive"), true);
    notes(p, u"lblMultstrReplNotes"_s,
          tr("Notes:") + u"<BR>"_s + tr("- Use Ctrl+Plus (+) and Ctrl+Minus (-) to insert or delete a line at current position"));

    // first run: install the predefined set shipped with the original program
    QSettings s = appSettings();
    if (!s.value(u"multstr/initialized"_s).toBool()) {
        QFile f(u":/default.xml"_s);
        if (f.open(QIODevice::ReadOnly)) {
            QXmlStreamReader r(&f);
            QString name;
            while (!r.atEnd()) {
                r.readNext();
                if (r.isStartElement() && r.name() == u"Set") {
                    name = r.attributes().value(u"Name"_s).toString();
                } else if (r.isCDATA() && !name.isEmpty()) {
                    s.setValue(u"multstr/sets/"_s + name, r.text().toString().trimmed());
                }
            }
        }
        s.setValue(u"multstr/initialized"_s, true);
    }
    multstrSets();
    connect(setName_, &QComboBox::activated, this, [this](int) {
        const QString t = appSettings().value(u"multstr/sets/"_s + setName_->currentText()).toString();
        setTableText(t);
        emit changed();
    });
    fields_.push_back(
        {p.name + u"/Table"_s, [this] { return tableText(); }, [this](const QVariant& v) { setTableText(v.toString()); }});
    fields_.push_back({p.name + u"/SetName"_s, [this] { return setName_->currentText(); },
                       [this](const QVariant& v) { setName_->setCurrentText(v.toString()); }});
}

void ActionPanel::multstrSets() {
    QSettings s = appSettings();
    s.beginGroup(u"multstr/sets"_s);
    const QString cur = setName_->currentText();
    setName_->clear();
    setName_->addItems(s.childKeys());
    setName_->setCurrentText(cur);
    s.endGroup();
}

QString ActionPanel::tableText() const {
    QString t;
    for (int r = 0; r < table_->rowCount(); ++r) {
        const QString a = table_->item(r, 0) ? table_->item(r, 0)->text() : QString();
        const QString b = table_->item(r, 1) ? table_->item(r, 1)->text() : QString();
        if (!a.isEmpty()) {
            t += a + u'\t' + b + u'\n';
        }
    }
    return t;
}

void ActionPanel::setTableText(const QString& text) {
    QSignalBlocker block(table_);
    table_->clearContents();
    const QStringList lines = text.split(QRegularExpression(u"\r\n|\n|\r"_s), Qt::SkipEmptyParts);
    table_->setRowCount(int(lines.size()) + 1);
    for (int i = 0; i < lines.size(); ++i) {
        const int tab = int(lines[i].indexOf(u'\t'));
        table_->setItem(i, 0, new QTableWidgetItem(tab < 0 ? lines[i] : lines[i].left(tab)));
        table_->setItem(i, 1, new QTableWidgetItem(tab < 0 ? QString() : lines[i].mid(tab + 1)));
    }
}

// --- take names from list ---------------------------------------------------------------------------
void ActionPanel::buildFromList(Page& p) {
    W& w = *w_;
    listEdit_ = new QPlainTextEdit;
    p.v->addWidget(listEdit_, 1);
    connect(listEdit_, &QPlainTextEdit::textChanged, this, &ActionPanel::changed);
    auto* btns = new QHBoxLayout;
    auto add = [&](const QString& t, auto fn) {
        auto* b = new QPushButton(t);
        connect(b, &QPushButton::clicked, this, fn);
        btns->addWidget(b);
    };
    add(tr("Clear"), [this] { listEdit_->clear(); });
    add(tr("Open..."), [this] {
        const QString f = QFileDialog::getOpenFileName(this, tr("Open"), {}, tr("Text files (*.txt);;All files (*.*)"));
        QFile file(f);
        if (!f.isEmpty() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            listEdit_->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });
    add(tr("Save..."), [this] {
        const QString f = QFileDialog::getSaveFileName(this, tr("Save"), {}, tr("Text files (*.txt);;All files (*.*)"));
        QFile file(f);
        if (!f.isEmpty() && file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(listEdit_->toPlainText().toUtf8());
        }
    });
    btns->addStretch();
    p.v->addLayout(btns);
    w.flAppend = check(p, u"AppendExt"_s, tr("Append original extension of the file"), false);
    w.flOnlyExt = check(p, u"OnlyExt"_s, tr("Use items as extensions instead of names"), false);
    notes(p, u"lblFromListNotes"_s,
          tr("Notes:") + u"<BR>"_s +
              tr("- Each line from this list will be used as a name for a file. Items are used sequentially<BR>"
                 "- If lines are empty or if there are not enough lines, the corresponding files will not be renamed"));
    fields_.push_back({p.name + u"/List"_s, [this] { return listEdit_->toPlainText(); },
                       [this](const QVariant& v) { listEdit_->setPlainText(v.toString()); }});
}

// --- building the action ----------------------------------------------------------------------------
static int checkedIndex(const std::vector<QRadioButton*>& r) {
    for (size_t i = 0; i < r.size(); ++i) {
        if (r[i]->isChecked()) {
            return int(i);
        }
    }
    return 0;
}

std::unique_ptr<Action> ActionPanel::makeAction(bool updateHistory) {
    const int idx = currentIndex();
    if (idx < 0) {
        return nullptr;
    }
    W& w = *w_;
    auto remember = [&](HistoryCombo* c) {
        if (updateHistory) {
            c->remember();
        }
    };
    auto act = ActionList::create(ActionType(idx));
    switch (ActionType(idx)) {
        case ActionType::ChangeExt: {
            auto& a = static_cast<ActionChangeExt&>(*act);
            a.newExtension = w.extNew->currentText();
            if (!a.newExtension.isEmpty() && a.newExtension[0] != u'.') {
                a.newExtension.prepend(u'.');
            }
            if (a.newExtension == u".") {
                a.newExtension.clear();
            }
            a.noReplace = w.extNoRepl->isChecked();
            remember(w.extNew);
            break;
        }
        case ActionType::StringRepl: {
            auto& a = static_cast<ActionStringRepl&>(*act);
            a.search = w.srcSearch->currentText();
            a.replace = w.srcRepl->currentText();
            a.all = w.srAll->isChecked();
            a.caseSens = w.srCase->isChecked();
            a.inclExt = w.srExt->isChecked();
            a.onlyExt = w.srOnlyExt->isChecked();
            a.wildcards = w.srWild->isChecked();
            remember(w.srcSearch);
            remember(w.srcRepl);
            break;
        }
        case ActionType::MultstrRepl: {
            auto& a = static_cast<ActionMultstrRepl&>(*act);
            a.setName = setName_->currentText();
            a.setChars = tableText();
            a.includeExt = w.msExt->isChecked();
            a.caseSens = w.msCase->isChecked();
            break;
        }
        case ActionType::StringInsert: {
            auto& a = static_cast<ActionStringIns&>(*act);
            a.newString = w.insStr->currentText();
            a.pos = w.insPos->value();
            a.fromBegin = checkedIndex(w.insDir) == 0;
            a.ext = w.insExt->isChecked();
            remember(w.insStr);
            break;
        }
        case ActionType::MoveString: {
            auto& a = static_cast<ActionMoveStr&>(*act);
            a.fromPos = w.mvFrom->value();
            a.fromBegin = checkedIndex(w.mvFrom_) == 0;
            a.count = w.mvCount->value();
            a.toPos = w.mvTo->value();
            a.toBegin = checkedIndex(w.mvTo_) == 0;
            break;
        }
        case ActionType::CharDel: {
            auto& a = static_cast<ActionCharDel&>(*act);
            a.nbChar = w.delNb->value();
            a.afterString = checkedIndex(w.delWhere);
            a.pos = w.delPos->value();
            a.fromBegin = checkedIndex(w.delDir) == 0;
            a.str = w.delStr->currentText();
            a.ext = w.delExt->isChecked();
            remember(w.delStr);
            break;
        }
        case ActionType::Enum: {
            auto& a = static_cast<ActionEnum&>(*act);
            a.mask = w.enumMask->currentText();
            a.startAt = w.enumStart->value();
            a.digits = w.enumDigits->value();
            a.increment = w.enumIncr->value();
            a.restart = w.enumRestart->isChecked();
            a.sharePairs = w.enumShare->isChecked();
            remember(w.enumMask);
            break;
        }
        case ActionType::MP3Tag: {
            auto& a = static_cast<ActionMP3&>(*act);
            a.mask = w.mp3Mask->currentText();
            a.twoDigit = w.mp3Two->isChecked();
            remember(w.mp3Mask);
            break;
        }
        case ActionType::DateTime: {
            auto& a = static_cast<ActionDateTime&>(*act);
            a.mask = w.dtMask->currentText();
            a.addSuffix = w.dtSuffix->isChecked();
            a.whichDate = checkedIndex(w.dtWhich) == 0 ? 0 : 1;
            a.offset = w.dtOffset->value();
            remember(w.dtMask);
            break;
        }
        case ActionType::Random: {
            auto& a = static_cast<ActionRandom&>(*act);
            a.mask = w.rndMask->currentText();
            a.method = checkedIndex(w.rndMethod);
            remember(w.rndMask);
            break;
        }
        case ActionType::Case: {
            auto& a = static_cast<ActionCase&>(*act);
            a.caseType = checkedIndex(w.caseType);
            a.afterChars = w.caseAfter->currentText();
            a.useLocale = w.caseLocale->isChecked();
            a.includeExt = w.caseIncl->isChecked();
            a.onlyExt = w.caseOnly->isChecked();
            remember(w.caseAfter);
            break;
        }
        case ActionType::FromList: {
            auto& a = static_cast<ActionFromList&>(*act);
            a.listContents = listEdit_->toPlainText();
            a.appendExt = w.flAppend->isChecked();
            a.onlyExt = w.flOnlyExt->isChecked();
            break;
        }
        case ActionType::Regexp: {
            auto& a = static_cast<ActionRegexp&>(*act);
            a.expr = w.reExpr->currentText();
            a.repl = w.reRepl->currentText();
            remember(w.reExpr);
            remember(w.reRepl);
            break;
        }
        case ActionType::Exif: {
            auto& a = static_cast<ActionExif&>(*act);
            a.mask = w.exifMask->currentText();
            a.offset = w.exifOffset->value();
            a.numStart = w.exStart->value();
            a.numDigits = w.exDigits->value();
            a.numIncr = w.exIncr->value();
            a.numRestart = w.exRestart->isChecked();
            a.numSharePairs = w.exShare->isChecked();
            remember(w.exifMask);
            break;
        }
        default:
            return nullptr;
    }
    return act;
}

// Reverse of makeAction(): puts the settings of `act` back into the controls (to edit an action of the batch).
void ActionPanel::setFromAction(const Action& act) {
    W& w = *w_;
    auto radio = [](std::vector<QRadioButton*>& r, int i) {
        if (i >= 0 && i < int(r.size())) {
            r[size_t(i)]->setChecked(true);
        }
    };
    selectAction(int(act.type()));
    switch (act.type()) {
        case ActionType::ChangeExt: {
            auto& a = static_cast<const ActionChangeExt&>(act);
            w.extNew->setCurrentText(a.newExtension.startsWith(u'.') ? a.newExtension.mid(1)
                                                                     : a.newExtension);  // shown without the dot
            w.extNoRepl->setChecked(a.noReplace);
            break;
        }
        case ActionType::StringRepl: {
            auto& a = static_cast<const ActionStringRepl&>(act);
            w.srcSearch->setCurrentText(a.search);
            w.srcRepl->setCurrentText(a.replace);
            w.srAll->setChecked(a.all);
            w.srCase->setChecked(a.caseSens);
            w.srExt->setChecked(a.inclExt);
            w.srOnlyExt->setChecked(a.onlyExt);
            w.srWild->setChecked(a.wildcards);
            break;
        }
        case ActionType::MultstrRepl: {
            auto& a = static_cast<const ActionMultstrRepl&>(act);
            setName_->setCurrentText(a.setName);
            setTableText(a.setChars);
            w.msExt->setChecked(a.includeExt);
            w.msCase->setChecked(a.caseSens);
            break;
        }
        case ActionType::StringInsert: {
            auto& a = static_cast<const ActionStringIns&>(act);
            w.insStr->setCurrentText(a.newString);
            w.insPos->setValue(a.pos);
            radio(w.insDir, a.fromBegin ? 0 : 1);
            w.insExt->setChecked(a.ext);
            break;
        }
        case ActionType::MoveString: {
            auto& a = static_cast<const ActionMoveStr&>(act);
            w.mvFrom->setValue(a.fromPos);
            radio(w.mvFrom_, a.fromBegin ? 0 : 1);
            w.mvCount->setValue(a.count);
            w.mvTo->setValue(a.toPos);
            radio(w.mvTo_, a.toBegin ? 0 : 1);
            break;
        }
        case ActionType::CharDel: {
            auto& a = static_cast<const ActionCharDel&>(act);
            w.delNb->setValue(a.nbChar);
            radio(w.delWhere, a.afterString);
            w.delPos->setValue(a.pos);
            radio(w.delDir, a.fromBegin ? 0 : 1);
            w.delStr->setCurrentText(a.str);
            w.delExt->setChecked(a.ext);
            break;
        }
        case ActionType::Enum: {
            auto& a = static_cast<const ActionEnum&>(act);
            w.enumMask->setCurrentText(a.mask);
            w.enumStart->setValue(a.startAt);
            w.enumDigits->setValue(a.digits);
            w.enumIncr->setValue(a.increment);
            w.enumRestart->setChecked(a.restart);
            w.enumShare->setChecked(a.sharePairs);
            break;
        }
        case ActionType::MP3Tag: {
            auto& a = static_cast<const ActionMP3&>(act);
            w.mp3Mask->setCurrentText(a.mask);
            w.mp3Two->setChecked(a.twoDigit);
            break;
        }
        case ActionType::DateTime: {
            auto& a = static_cast<const ActionDateTime&>(act);
            w.dtMask->setCurrentText(a.mask);
            w.dtSuffix->setChecked(a.addSuffix);
            radio(w.dtWhich, a.whichDate == 0 ? 0 : 1);
            w.dtOffset->setValue(a.offset);
            break;
        }
        case ActionType::Random: {
            auto& a = static_cast<const ActionRandom&>(act);
            w.rndMask->setCurrentText(a.mask);
            radio(w.rndMethod, a.method);
            break;
        }
        case ActionType::Case: {
            auto& a = static_cast<const ActionCase&>(act);
            radio(w.caseType, a.caseType);
            w.caseAfter->setCurrentText(a.afterChars);
            w.caseLocale->setChecked(a.useLocale);
            w.caseIncl->setChecked(a.includeExt);
            w.caseOnly->setChecked(a.onlyExt);
            break;
        }
        case ActionType::FromList: {
            auto& a = static_cast<const ActionFromList&>(act);
            listEdit_->setPlainText(a.listContents);
            w.flAppend->setChecked(a.appendExt);
            w.flOnlyExt->setChecked(a.onlyExt);
            break;
        }
        case ActionType::Regexp: {
            auto& a = static_cast<const ActionRegexp&>(act);
            w.reExpr->setCurrentText(a.expr);
            w.reRepl->setCurrentText(a.repl);
            break;
        }
        case ActionType::Exif: {
            auto& a = static_cast<const ActionExif&>(act);
            w.exifMask->setCurrentText(a.mask);
            w.exifOffset->setValue(a.offset);
            w.exStart->setValue(a.numStart);
            w.exDigits->setValue(a.numDigits);
            w.exIncr->setValue(a.numIncr);
            w.exRestart->setChecked(a.numRestart);
            w.exShare->setChecked(a.numSharePairs);
            break;
        }
        default:
            break;
    }
    emit changed();
}

// --- persistence -----------------------------------------------------------------------------------
void ActionPanel::loadState() {
    QSettings s = appSettings();
    for (const Field& f : fields_) {
        const QString k = u"last/"_s + f.key;
        if (s.contains(k)) {
            f.set(s.value(k));
        }
    }
    for (size_t i = 0; i < combos_.size(); ++i) {
        const QStringList h = s.value(u"history/%1"_s.arg(i)).toStringList();
        const QString cur = combos_[i]->currentText();
        combos_[i]->clear();
        combos_[i]->addItems(h);
        combos_[i]->setCurrentText(cur);
    }
    list_->setCurrentRow(s.value(u"last/action"_s, 0).toInt());
}

void ActionPanel::saveState() const {
    QSettings s = appSettings();
    for (const Field& f : fields_) {
        s.setValue(u"last/"_s + f.key, f.get());
    }
    for (size_t i = 0; i < combos_.size(); ++i) {
        QStringList h;
        for (int k = 0; k < combos_[i]->count(); ++k) {
            h << combos_[i]->itemText(k);
        }
        s.setValue(u"history/%1"_s.arg(i), h);
    }
    s.setValue(u"last/action"_s, list_->currentRow());
}

void ActionPanel::applyOptions() {
    for (HistoryCombo* c : combos_) {
        if (appOptions().autoComplete) {
            if (!c->completer()) {
                auto* comp = new QCompleter(c->model(), c);
                comp->setCaseSensitivity(Qt::CaseInsensitive);
                c->setCompleter(comp);
            }
        } else {
            c->setCompleter(nullptr);
        }
        while (c->count() > qMax(1, appOptions().historyMax)) {
            c->removeItem(c->count() - 1);
        }
    }
}

bool ActionPanel::eventFilter(QObject* o, QEvent* e) {
    if (o == table_ && e->type() == QEvent::KeyPress) {
        auto* k = static_cast<QKeyEvent*>(e);
        if (k->modifiers() & Qt::ControlModifier) {
            const int row = qMax(0, table_->currentRow());
            if (k->key() == Qt::Key_Plus || k->key() == Qt::Key_Equal) {
                table_->insertRow(row);
                emit changed();
                return true;
            }
            if ((k->key() == Qt::Key_Minus) && table_->rowCount() > 1) {
                table_->removeRow(row);
                emit changed();
                return true;
            }
        }
    }
    return QWidget::eventFilter(o, e);
}
