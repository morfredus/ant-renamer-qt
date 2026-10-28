#pragma once
// Environment variables for developers (ANTREN_MISSING, ANTREN_SHOT and the ones that go with it, see docs/BUILDING.md):
// list the untranslated texts, take screenshots. Does nothing when none of them is set.
class MainWindow;
class QApplication;

void runDeveloperAids(MainWindow& w, QApplication& app);
