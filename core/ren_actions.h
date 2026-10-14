#pragma once
// Port of Actions.pas. Settings are plain public fields so a UI can bind to them directly.
#include "ren_files.h"
#include "ren_id3.h"

#include <QMap>
#include <QString>
#include <QXmlStreamWriter>
#include <functional>
#include <memory>
#include <vector>

namespace ren {

// Numeric values are the ones stored by the Delphi version (and used by its UI).
enum class ActionType {
    Unknown = -1, ChangeExt = 0, StringRepl, MultstrRepl, StringInsert, MoveString, CharDel,
    Enum, MP3Tag, DateTime, Random, Case, FromList, Regexp, Exif
};

enum class Op { Rename, Preview, Revert };

// (file just handled, position of the action in the batch, index of the file)
using Progress = std::function<void(File&, int batchPos, int filePos)>;

// What an action reads from / writes to the batch file: one element, attributes + child texts.
struct XmlElem {
    QString name;
    QMap<QString, QString> attrs;
    QMap<QString, QString> childText;
    QString str(const QString& k, const QString& def) const { return attrs.value(k, def); }
    int num(const QString& k, int def) const;
    bool flag(const QString& k, bool def) const;
};

class Action {
public:
    explicit Action(ActionType t = ActionType::Unknown) : type_(t) {}
    virtual ~Action() = default;

    ActionType type() const { return type_; }
    int position = 0;

    void perform(FileList& list, Op op, const Progress& progress = {});
    virtual QString description() const;
    virtual void save(QXmlStreamWriter&) const {}
    virtual void load(const XmlElem&) {}

    // The new name this action would give to `fileName` (a full path). Public for tests.
    virtual QString makeNewName(const QString& fileName, bool isFolder);

protected:
    virtual bool getTag(const QString& tag, QString& value);
    virtual void init() {}
    virtual void finish() {}

    bool plainFolder() const { return isFolder_ && !options().folderExt; }  // folder without extension
    QString sourcePath() const { return curFile_ ? curFile_->filePath() : fileName_; }
    void setInfo(const QString& what);  // "<file> was not renamed : <what>", status Info

    ActionType type_;
    QString fileName_;
    bool isFolder_ = false;
    Error lastError_;
    File* curFile_ = nullptr;
};

class ActionChangeExt : public Action {
public:
    ActionChangeExt() : Action(ActionType::ChangeExt) {}
    QString newExtension = QStringLiteral(".ext");
    bool noReplace = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionStringRepl : public Action {
public:
    ActionStringRepl() : Action(ActionType::StringRepl) {}
    QString search = QStringLiteral("old"), replace = QStringLiteral("new");
    bool all = true, caseSens = true, inclExt = false, onlyExt = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionMultstrRepl : public Action {
public:
    ActionMultstrRepl() : Action(ActionType::MultstrRepl) {}
    QString setName, setChars;  // setChars: one "from<TAB>to" per line
    bool includeExt = false, caseSens = true;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionStringIns : public Action {
public:
    ActionStringIns() : Action(ActionType::StringInsert) {}
    QString newString = QStringLiteral("new");
    int pos = 0;
    bool fromBegin = true, ext = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionMoveStr : public Action {
public:
    ActionMoveStr() : Action(ActionType::MoveString) {}
    int fromPos = 0, count = 1, toPos = 0;
    bool fromBegin = true, toBegin = true;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionCharDel : public Action {
public:
    ActionCharDel() : Action(ActionType::CharDel) {}
    int nbChar = 1;
    int afterString = 0;  // 0 = at position, 1 = after str, 2 = before str
    int pos = 0;
    bool fromBegin = true, ext = false;
    QString str = QStringLiteral("str");
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionEnum : public Action {
public:
    ActionEnum() : Action(ActionType::Enum) {}
    QString mask = QStringLiteral("File %num%%ext%");
    int startAt = 1, digits = 3, increment = 1;
    bool restart = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
protected:
    bool getTag(const QString&, QString&) override;
    void init() override { curPos_ = startAt; prevFolder_.clear(); }
private:
    int curPos_ = 0;
    QString prevFolder_;
};

class ActionMP3 : public Action {
public:
    ActionMP3() : Action(ActionType::MP3Tag) {}
    QString mask = QStringLiteral("%author% - %title%%ext%");
    bool twoDigit = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
protected:
    bool getTag(const QString&, QString&) override;
private:
    Id3Info id3_;
};

class ActionDateTime : public Action {
public:
    ActionDateTime() : Action(ActionType::DateTime) {}
    QString mask = QStringLiteral("yyyy'-'mm'-'dde");  // Delphi FormatDateTime; f = name, e = extension
    bool addSuffix = false;
    int whichDate = 1;  // 0 = creation, 1 = modification
    int offset = 0;     // seconds
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionRandom : public Action {
public:
    ActionRandom() : Action(ActionType::Random) {}
    QString mask = QStringLiteral("%random%%ext%");
    int method = 0;  // 0 = numbers, 1 = tick counter, 2 = GUID
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
protected:
    bool getTag(const QString&, QString&) override;
};

class ActionCase : public Action {
public:
    ActionCase() : Action(ActionType::Case) {}
    int caseType = 0;  // 0 = words, 1 = first letter, 2 = upper, 3 = lower
    QString afterChars = QStringLiteral("- .+(");
    bool useLocale = true, includeExt = false, onlyExt = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionFromList : public Action {
public:
    ActionFromList() : Action(ActionType::FromList) {}
    QString listContents;  // one new name per line
    bool appendExt = false, onlyExt = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
protected:
    void init() override;
private:
    QStringList list_;
};

class ActionRegexp : public Action {
public:
    ActionRegexp() : Action(ActionType::Regexp) {}
    QString expr, repl;  // repl: $1 / ${1} / $& for groups, \n \t for control chars
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
};

class ActionExif : public Action {
public:
    ActionExif() : Action(ActionType::Exif) {}
    QString mask;
    int offset = 0;  // seconds, applied to date-time tags
    // %num% counter, same meaning as in the Enumeration action
    int numStart = 1, numDigits = 3, numIncr = 1;
    bool numRestart = false;
    QString description() const override;
    QString makeNewName(const QString&, bool) override;
    void save(QXmlStreamWriter&) const override;
    void load(const XmlElem&) override;
protected:
    bool getTag(const QString&, QString&) override;
    void init() override { curNum_ = numStart; prevFolder_.clear(); }
private:
    QMap<QString, QString> tags_;
    int curNum_ = 0;
    QString prevFolder_;
};

class ActionList {
public:
    Action& add(ActionType t);
    Action& add(std::unique_ptr<Action> a);  // takes ownership, appended at the end
    size_t size() const { return items_.size(); }
    Action& at(size_t i) { return *items_[i]; }
    void clear() { items_.clear(); }
    void replace(size_t i, std::unique_ptr<Action> a) {  // edit in place: same position in the batch
        a->position = int(i);
        items_[i] = std::move(a);
    }
    void remove(size_t i) {
        items_.erase(items_.begin() + i);
        for (size_t k = 0; k < items_.size(); ++k) items_[k]->position = int(k);
    }
    void sort();  // by position
    void perform(FileList& files, Op op, const Progress& progress = {});
    bool save(const QString& path) const;
    bool load(const QString& path);

    static std::unique_ptr<Action> create(ActionType t);

private:
    std::vector<std::unique_ptr<Action>> items_;
};

}  // namespace ren
