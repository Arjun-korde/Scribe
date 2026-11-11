#pragma once

#include "FileDialogs.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

namespace scribe::editor
{
class EditorTabs;
class EditorWidget;
} // namespace scribe::editor

namespace scribe::app
{

/// Notices when another program changes or deletes a file that is open in a tab, and asks the
/// user what to do (reload, keep, or close the tab), one question at a time.
///
/// A file is checked by comparing its FileStamp with the one its document recorded when it was
/// loaded or saved, so Scribe's own saves never count as changes. Checks run when the file
/// system watcher reports something (after the events stop for kDebounceMs, so a program that is
/// still writing is not interrupted) and whenever the application becomes active again, which also
/// covers network drives where the watcher is unreliable. The watcher watches each file and its
/// folder, because a program that saves by replacing the file ends the watch on the file itself.
class ExternalChangeMonitor : public QObject
{
    Q_OBJECT

public:
    static constexpr int kDebounceMs = 300;

    /// `dialogParent` is the window the questions belong to; it may be null (used by tests).
    ExternalChangeMonitor(scribe::editor::EditorTabs& tabs, FileDialogs dialogs,
                          QWidget* dialogParent, QObject* parent = nullptr);

    /// Checks every open file now and asks about each one that changed. Returns when all
    /// questions are answered. A call made while a question is open is remembered and runs after.
    void checkNow();

    /// True while a question is on screen.
    bool isAsking() const { return m_asking; }

private:
    /// Makes the watcher watch exactly the open files and their folders.
    void syncWatches();
    void scheduleCheck();
    void handleChange(scribe::editor::EditorWidget& editor);

    scribe::editor::EditorTabs& m_tabs;
    FileDialogs m_dialogs;
    QWidget* m_dialogParent;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    bool m_asking = false;
    bool m_recheck = false;
};

} // namespace scribe::app
