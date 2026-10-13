#pragma once
#include <QIcon>
#include <QStringList>

// Indices into the original Scrows toolbar strip (same numbers as tbi* in ConstValues.pas).
enum TbIcon {
    TbTabFiles = 0, TbTabActions, TbTabLog, TbAddFiles, TbAddFolder, TbRemoveSelected, TbRemoveAll,
    TbRemoveDead, TbPreview, TbMoveUp, TbMoveDown, TbMoveTop, TbMoveBottom, TbBatchAdd,
    TbBatchRemoveSel, TbBatchClear, TbBatchOpen, TbBatchSave, TbLogClear, TbLogSave, TbLogCopy,
    TbLogFilter, TbGo, TbStop, TbUndo, TbAlwaysOnTop, TbOptions, TbHelp, TbAbout, TbExit
};

QIcon tbIcon(int index);
void setIconSet(int set);   // 0 Scrows, 1 Windows XP, 2 Windows XP Large
int iconSetCount();
QStringList iconSetNames();
int iconCell();             // pixel size of the current set

