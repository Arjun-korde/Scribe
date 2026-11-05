#pragma once

#include "Document.h"

#include <QTabWidget>

#include <memory>

namespace scribe::editor
{

class EditorWidget;

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
    EditorWidget* editorAt(int index) const;

    /// Adds an empty tab at the end and makes it active.
    EditorWidget* addEditor();

    /// Index of the tab showing the file at `path`, or -1. Different spellings of the same path
    /// count as the same file.
    int indexOfPath(const QString& path) const;

    /// Opens a file. Switches to its tab if it is already open (keeping any edits), reuses the
    /// active tab if it is an untouched empty "Untitled", and otherwise opens in a new tab. A
    /// failed open changes nothing: no tab is added and the active tab keeps its content.
    core::IoResult openFile(const QString& path);

    /// These close tabs without asking about unsaved changes; callers that care must ask first.
    void closeTab(int index);
    void closeOtherTabs(int index);
    void closeAllTabs();

    /// Cycle through the tabs, wrapping around at both ends.
    void nextTab();
    void previousTab();

signals:
    /// Emitted when the active tab changes or the active editor's state (name, modified, encoding,
    /// line ending) changes. Changes in background tabs do not emit it.
    void currentEditorStateChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    EditorWidget* adopt(std::unique_ptr<EditorWidget> editor);
    void refreshTab(const EditorWidget* editor);
    void removeAndDelete(QWidget* page);
    static bool isUntouchedUntitled(const EditorWidget& editor);
};

} // namespace scribe::editor
