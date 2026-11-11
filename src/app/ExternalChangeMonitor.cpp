#include "ExternalChangeMonitor.h"

#include "EditorTabs.h"
#include "EditorWidget.h"
#include "FileStamp.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPointer>
#include <QSet>

using scribe::core::FileStamp;
using scribe::editor::EditorWidget;

namespace scribe::app
{

namespace
{

/// The watcher asks for the path of what to watch; this gives one spelling per file.
QString watchPathOf(const QString& filePath)
{
    return QFileInfo(filePath).absoluteFilePath();
}

bool differsFromDisk(const EditorWidget& editor)
{
    const QString& path = editor.fileDocument().filePath();
    return !path.isEmpty() && FileStamp::of(path) != editor.fileDocument().diskStamp();
}

} // namespace

ExternalChangeMonitor::ExternalChangeMonitor(scribe::editor::EditorTabs& tabs, FileDialogs dialogs,
                                             QWidget* dialogParent, QObject* parent)
    : QObject(parent)
    , m_tabs(tabs)
    , m_dialogs(std::move(dialogs))
    , m_dialogParent(dialogParent)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(kDebounceMs);
    connect(&m_debounce, &QTimer::timeout, this, &ExternalChangeMonitor::checkNow);

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this,
            &ExternalChangeMonitor::scheduleCheck);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &ExternalChangeMonitor::scheduleCheck);

    // New files, Save As, and closed tabs all change which files need watching.
    connect(&m_tabs, &scribe::editor::EditorTabs::fileUsed, this,
            &ExternalChangeMonitor::syncWatches);
    connect(&m_tabs, &scribe::editor::EditorTabs::currentEditorStateChanged, this,
            &ExternalChangeMonitor::syncWatches);

    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive) {
                    scheduleCheck();
                }
            });

    syncWatches();
}

void ExternalChangeMonitor::scheduleCheck()
{
    m_debounce.start(); // restarts, so a burst of events gives one check after it ends
}

void ExternalChangeMonitor::syncWatches()
{
    QSet<QString> wanted;
    for (int i = 0; i < m_tabs.count(); ++i) {
        const QString& path = m_tabs.editorAt(i)->fileDocument().filePath();
        if (path.isEmpty()) {
            continue;
        }
        const QString file = watchPathOf(path);
        wanted.insert(file);
        wanted.insert(QFileInfo(file).absolutePath());
    }

    const QStringList current = m_watcher.files() + m_watcher.directories();
    const QSet<QString> have(current.begin(), current.end());

    QStringList stale;
    for (const QString& path : have) {
        if (!wanted.contains(path)) {
            stale.append(path);
        }
    }
    if (!stale.isEmpty()) {
        m_watcher.removePaths(stale);
    }

    QStringList missing;
    for (const QString& path : wanted) {
        if (!have.contains(path)) {
            missing.append(path);
        }
    }
    if (!missing.isEmpty()) {
        m_watcher.addPaths(missing); // a path that does not exist (a deleted file) is skipped
    }
}

void ExternalChangeMonitor::checkNow()
{
    m_debounce.stop();
    if (m_asking) {
        m_recheck = true;
        return;
    }

    m_asking = true;
    do {
        m_recheck = false;

        QList<QPointer<EditorWidget>> changed;
        for (int i = 0; i < m_tabs.count(); ++i) {
            EditorWidget* editor = m_tabs.editorAt(i);
            if (differsFromDisk(*editor)) {
                changed.append(editor);
            }
        }

        for (const QPointer<EditorWidget>& editor : changed) {
            // An earlier question may have taken a while: the tab may be gone (a closed editor
            // lives on until the event loop deletes it), or the file may have changed back.
            if (editor && m_tabs.indexOf(editor) >= 0 && differsFromDisk(*editor)) {
                handleChange(*editor);
            }
        }
    } while (m_recheck);
    m_asking = false;

    syncWatches(); // a replaced file needs watching again
}

void ExternalChangeMonitor::handleChange(EditorWidget& editor)
{
    const QString path = editor.fileDocument().filePath();
    const ExternalChangeKind kind =
        FileStamp::of(path).exists ? ExternalChangeKind::Changed : ExternalChangeKind::Deleted;

    m_tabs.setCurrentWidget(&editor); // so the user can see which document is meant

    ExternalChangeChoice choice = ExternalChangeChoice::Keep;
    if (m_dialogs.confirmExternalChange) {
        choice = m_dialogs.confirmExternalChange(m_dialogParent, editor.displayName(), kind,
                                                 editor.isModified());
    }
    // Reload makes no sense for a missing file and Close none for an existing one.
    if ((choice == ExternalChangeChoice::Reload && kind == ExternalChangeKind::Deleted) ||
        (choice == ExternalChangeChoice::Close && kind == ExternalChangeKind::Changed)) {
        choice = ExternalChangeChoice::Keep;
    }

    switch (choice) {
        case ExternalChangeChoice::Reload: {
            const core::IoResult result = editor.reload();
            if (!result.ok()) {
                // Remember the file as it is now, so the same failure is not reported again and
                // again.
                editor.acknowledgeDiskChange();
                if (m_dialogs.showError) {
                    m_dialogs.showError(
                        m_dialogParent,
                        QCoreApplication::translate("ExternalChangeMonitor", "Reload"),
                        QCoreApplication::translate("ExternalChangeMonitor",
                                                    "Could not reload %1.\n\n%2")
                            .arg(QDir::toNativeSeparators(path), result.detail));
                }
            }
            break;
        }
        case ExternalChangeChoice::Close:
            // The close guard asks about unsaved changes; if the user cancels there, the tab stays
            // and must not be asked about again.
            if (!m_tabs.closeTab(m_tabs.indexOf(&editor))) {
                editor.keepAsUnsavedCopy();
            }
            break;
        case ExternalChangeChoice::Keep:
            if (kind == ExternalChangeKind::Deleted) {
                editor.keepAsUnsavedCopy();
            } else {
                editor.acknowledgeDiskChange();
            }
            break;
    }
}

} // namespace scribe::app
