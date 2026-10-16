#pragma once
// The "Actions" page of the original: list of action types on the left, the settings of the
// selected type on the right. It holds ONE action being configured; "Add to batch" snapshots it.
#include "ren_actions.h"

#include <QComboBox>
#include <QWidget>
#include <functional>
#include <memory>
#include <vector>

class QListWidget;
class QStackedWidget;
class QTableWidget;
class QVBoxLayout;
class QSpinBox;
class QCheckBox;
class QRadioButton;
class QPlainTextEdit;
class QLabel;

// Editable combo that remembers the values that were used.
class HistoryCombo : public QComboBox {
public:
    HistoryCombo();
    void remember();
};

class ActionPanel : public QWidget {
    Q_OBJECT
public:
    explicit ActionPanel(QWidget* parent = nullptr);
    ~ActionPanel() override;

    int currentIndex() const;
    void selectAction(int index);
    // A new action configured from the current controls; null if no type is selected.
    std::unique_ptr<ren::Action> makeAction(bool updateHistory);
    void setFromAction(const ren::Action& a);  // reverse of makeAction(): edit an action of the batch
    void loadState();
    void saveState() const;
    void applyOptions();  // history size / autocomplete

protected:
    bool eventFilter(QObject* o, QEvent* e) override;

signals:
    void changed();         // any control changed: the sample preview must be refreshed
    void helpRequested(const QString& topic);  // help page to open ("format", "regexp")

private:
    struct Field {
        QString key;
        std::function<QVariant()> get;
        std::function<void(const QVariant&)> set;
    };
    struct Page {
        QWidget* w;
        QVBoxLayout* v;
        QString name;
    };

    Page newPage(const QString& name);
    HistoryCombo* combo(Page& p, const QString& key, const QString& label, const QString& def, bool place = true);
    QSpinBox* spin(Page& p, const QString& key, const QString& label, int def, int min, int max,
                   const QString& suffix = {});
    QCheckBox* check(Page& p, const QString& key, const QString& label, bool def);
    void notes(Page& p, const QString& control, const QString& english);
    // Radio group; the result is the index of the checked one.
    std::vector<QRadioButton*> radios(Page& p, const QString& key, const QStringList& labels, int def,
                                      bool vertical = false, QWidget* firstExtra = nullptr);

    void buildPages();
    void buildMultstr(Page& p);
    void buildFromList(Page& p);
    void multstrSets();
    QString tableText() const;
    void setTableText(const QString& t);

    QListWidget* list_;
    QStackedWidget* pages_;
    std::vector<Field> fields_;
    std::vector<HistoryCombo*> combos_;
    QTableWidget* table_ = nullptr;
    QComboBox* setName_ = nullptr;
    QPlainTextEdit* listEdit_ = nullptr;
    // widgets read by makeAction()
    struct W;
    std::unique_ptr<W> w_;
};
