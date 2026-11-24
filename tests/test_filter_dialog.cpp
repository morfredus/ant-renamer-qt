// The "Filter files" dialog: ticking each criterion enables its fields (this crashed once: the lambda kept an initializer_list
// whose array was gone), and filter() reads back what was set.
#include "dialogs.h"

#include <QApplication>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QStandardPaths>
#include <QPushButton>

#include <cstdio>

using namespace Qt::StringLiterals;

static int failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

int main(int argc, char** argv) {
#ifndef Q_OS_WIN  // no display on the Linux CI; on Windows the deployed "windows" plugin is the only one and nothing is shown
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
#endif
    QStandardPaths::setTestModeEnabled(true);  // never touch the real settings
    QApplication app(argc, argv);
    FilterDialog dlg(nullptr);
    CHECK(dlg.filter().isEmpty());  // nothing ticked: no criterion
    const auto boxes = dlg.findChildren<QCheckBox*>();
    CHECK(boxes.size() == 5);
    for (QCheckBox* b : boxes) {  // tick and untick every box: the fields follow (the old code crashed on the first click)
        b->click();
        b->click();
        b->click();
    }
    const ren::FileFilter all = dlg.filter();
    CHECK(all.from && all.to && all.timeFrom && all.timeTo && all.minSize && all.maxSize);
    CHECK(all.minSize.value_or(0) == (1ll << 20));  // 1 MB by default
    for (QCheckBox* b : boxes) {
        b->click();
    }
    CHECK(dlg.filter().isEmpty());
    CHECK(dlg.keepMatching());

    // correcting the last filter: the dialog shows the criteria again, and offers the files it took out
    ren::FileFilter prev;
    prev.kind = ren::FileFilter::DateKind::Shot;
    prev.from = QDateTime(QDate(2024, 5, 1), QTime(8, 30));
    prev.timeFrom = QTime(14, 0);
    prev.timeTo = QTime(15, 0, 0, 999);
    prev.minSize = 3ll << 20;     // 3 MB
    prev.maxSize = 1500ll << 10;  // 1500 KB: not a whole MB
    FilterDialog again(nullptr, prev, false, 7);
    CHECK(!again.keepMatching());
    CHECK(again.putBack());  // ticked by default when files were taken out
    const ren::FileFilter back = again.filter();
    CHECK(back.kind == ren::FileFilter::DateKind::Shot);
    CHECK(back.from && *back.from == *prev.from);
    CHECK(!back.to);
    CHECK(back.timeFrom && *back.timeFrom == QTime(14, 0));
    CHECK(back.timeTo && back.timeTo->msecsSinceStartOfDay() == QTime(15, 0, 0, 999).msecsSinceStartOfDay());
    CHECK(back.minSize && *back.minSize == (3ll << 20));
    CHECK(back.maxSize && *back.maxSize == (1500ll << 10));
    CHECK(again.findChildren<QCheckBox*>().size() == 6);  // the five criteria and the put-back choice
    CHECK(!again.clearRequested());
    QPushButton* clearBtn = nullptr;  // "Clear the filter": only there when the last filter took files out
    for (auto* b : again.findChildren<QPushButton*>()) {
        if (b->text() == u"Clear the filter"_s) {
            clearBtn = b;
        }
    }
    CHECK(clearBtn != nullptr);
    if (clearBtn) {
        clearBtn->click();
        CHECK(again.clearRequested());
        CHECK(again.result() == QDialog::Accepted);
    }
    for (auto* b : dlg.findChildren<QPushButton*>()) {  // nothing to clear in a fresh dialog
        CHECK(b->text() != u"Clear the filter"_s);
    }
    CHECK(!dlg.clearRequested());
    std::printf(failures ? "%d FAILURE(S)\n" : "filter dialog checks passed\n", failures);
    return failures ? 1 : 0;
}
