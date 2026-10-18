#pragma once
#include <QIcon>
#include <QStringList>

// Indices into the original Scrows toolbar strip (same numbers as tbi* in ConstValues.pas).
enum TbIcon {
    TbTabFiles = 0, TbTabActions, TbTabLog, TbAddFiles, TbAddFolder, TbRemoveSelected, TbRemoveAll,
    TbRemoveDead, TbPreview, TbMoveUp, TbMoveDown, TbMoveTop, TbMoveBottom, TbBatchAdd,
    TbBatchRemoveSel, TbBatchClear, TbBatchOpen, TbBatchSave, TbLogClear, TbLogSave, TbLogCopy,
    TbLogFilter, TbGo, TbStop, TbUndo, TbAlwaysOnTop, TbOptions, TbHelp, TbAbout, TbExit,
    TbTheme,  // only drawn in code (light / dark toggle), whatever the icon set
    TbChangedOnly, TbShowHidden  // idem (view filters of the files list)
};

QIcon tbIcon(int index);
void setIconSet(int set);   // 0 Scrows, 1 Windows XP, 2 Windows XP Large
constexpr int kModernIconSet = 3;         // the set drawn in code (index in the Options list)
constexpr int kDefaultIconSet = kModernIconSet;
int iconSetCount();
QStringList iconSetNames();
int iconCell();             // pixel size of the current set

