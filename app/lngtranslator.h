#pragma once
// Reuses the original program's language files (Languages\*.lng) instead of Qt .ts files:
// every string of the UI is looked up by its English text, which is matched against
// English.lng and answered with the same entry of the chosen language file.
#include <QHash>
#include <QList>
#include <QString>
#include <QTranslator>

struct LangInfo {
    QString file;       // e.g. "French" (file name without .lng)
    QString localName;  // e.g. "Français"
    QString version, authors, comments;
};

QList<LangInfo> availableLanguages(const QString& dir);
QString dataPath(const QString& name);  // "Languages" / "help": next to the program, or in ../share/antrenamer-qt
QString languageDir();                  // <exe>/Languages
QString defaultLanguage();
QString helpLanguageCode();  // "en", "fr", "hr" or "ta": the languages the help pages exist in
void openHelp(
    const QString&
        topic);  // topic = page name without language: "index", "format", "regexp", ...             // from the system locale, "English" if unknown

class LngTranslator : public QTranslator {
public:
    bool loadLanguage(const QString& dir, const QString& language);
    QString translate(const char* context, const char* sourceText, const char* disambiguation = nullptr,
                      int n = -1) const override;
    bool isEmpty() const override { return map_.isEmpty(); }

    // Text of a named control (e.g. "lblEnumNotes") in the loaded language, or `fallback`.
    QString control(const QString& name, const QString& fallback) const;

private:
    QHash<QString, QString> map_;  // normalized English -> translated
    QString language_;
    QHash<QString, QString> controls_;  // control name -> translated Caption
};

// The installed translator, or null when running in English.
LngTranslator* lngTranslator();
void setLngTranslator(LngTranslator* t);
// Loads `language` in the installed translator (installs one if the program started in English).
void switchLanguage(const QString& language);
