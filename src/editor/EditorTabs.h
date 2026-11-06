#pragma once

#include "Document.h"

#include <QTabWidget>

#include <functional>
#include <memory>

namespace scribe::editor
{

class EditorWidget;
class FileDropFilter;

/// The row of document tabs. Each tab holds one EditorWidget. There is always at least one tab:
/// closing the last one leaves a fresh, empty "Untitled" tab.
///
/// A tab shows the file name, followed by " *" while its document has unsaved changes.
class EditorTabs : public QTabWidget
{
    Q_OBJECT

public:
    explicit EditorTabs(QWidget* parent = nullptr);

    using QTabWidget::tabBar;

    /// The editor of the active tab. Never null.
    EditorWidget* currentEditor() const;

    /// The editor in tab `index`, or nullptr for an invalid index.
    [[nodiscard]] EditorWidget* editorAt(int index) const;

    /// Adds an empty tab at the end and makes it active.
    EditorWidget* addEditor();

    /// Index of the tab showing the file at `path`, or -1. Different spellings of the same path
    /// count as the same file.
    [[nodiscard]] int indexOfPath(const QString& path) const;

    /// Opens a file. Switches to its tab if it is already open (keeping any edits), reuses the
    /// active tab if it is an untouched empty "Untitled", and otherwise opens in a new tab. A
    /// failed open changes nothing: no tab is added and the active tab keeps its content.
    core::IoResult openFile(const QString& path);

    /// Called for a modified editor that is about to be closed, with its tab already active.
    /// Returns true to let it close (for example after saving or discarding) and false to keep it.
    using CloseGuard = std::function<bool(EditorWidget&)>;

    /// Without a guard (the default) tabs close without asking. Unmodified tabs never ask.
    void setCloseGuard(CloseGuard guard);

    /// Closing goes left to right and stops at the first tab the guard refuses; tabs closed before
    /// that stay closed. Each returns true if everything requested was closed, false otherwise
    /// (including an invalid index). Whatever happens, at least one tab remains.
    bool closeTab(int index);
    bool closeOtherTabs(int index);
    bool closeAllTabs();

    /// Cycle through the tabs, wrapping around at both ends.
    void nextTab();
    void previousTab();

signals:
    /// Emitted when the active tab changes or the active editor's state (name, modified, encoding,
    /// line ending) changes. Changes in background tabs do not emit it.
    void currentEditorStateChanged();

    /// Files were dropped on the tab strip or on any editor, in the order they were dragged. Only
    /// local, existing files are reported. The tabs do not open them: whoever connects decides.
    /// Emitted after the drop has returned, so it is safe to show dialogs from the slot.
    void filesDropped(const QStringList& paths);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    EditorWidget* adopt(std::unique_ptr<EditorWidget> editor);
    void refreshTab(const EditorWidget* editor);
    void removeAndDelete(QWidget* page);
    QList<QWidget*> pages() const;

    /// Asks the guard about a modified tab (making it the active tab first, so the user can see
    /// what is being asked about). Unmodified tabs, and a missing guard, always allow closing.
    bool confirmClose(QWidget* page);

    static bool isUntouchedUntitled(const EditorWidget& editor);

    CloseGuard m_closeGuard;
    FileDropFilter* m_dropFilter = nullptr; // owned by this widget
};

} // namespace scribe::editor
