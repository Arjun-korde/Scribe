#pragma once

#include "Document.h"
#include "FileDialogs.h"

#include <QCoreApplication>

class QWidget;

namespace scribe::editor
{
class EditorTabs;
class EditorWidget;
} // namespace scribe::editor

/// What happens when the user opens, saves or closes documents: it asks for paths, runs the
/// operation on the editor, explains failures, and decides what to do with unsaved changes.
/// The main window only wires menus to it.
///
/// The controller installs itself as the tabs' close guard, so every way of closing a modified tab
/// (menu, tab button, middle-click, Close Others, Close All, closing the window) asks first.
class DocumentController
{
    Q_DECLARE_TR_FUNCTIONS(DocumentController)

public:
    /// `dialogParent` is the window the dialogs belong to; it may be null (used by tests).
    DocumentController(scribe::editor::EditorTabs& tabs, FileDialogs dialogs,
                       QWidget* dialogParent);
    ~DocumentController();

    DocumentController(const DocumentController&) = delete;
    DocumentController& operator=(const DocumentController&) = delete;

    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();

private:
    /// Saves to the editor's path, or asks for one if it has none. Returns true only if the file
    /// was written; a cancelled dialog or a failure returns false.
    bool save(scribe::editor::EditorWidget& editor);
    bool saveAs(scribe::editor::EditorWidget& editor);

    /// The close guard: asks what to do with `editor`'s unsaved changes. Returns true if the
    /// editor may be closed (changes saved or deliberately discarded).
    bool confirmClose(scribe::editor::EditorWidget& editor);

    void showIoError(const scribe::core::IoResult& result, const QString& path,
                     const scribe::editor::EditorWidget& editor);

    scribe::editor::EditorTabs& m_tabs;
    FileDialogs m_dialogs;
    QWidget* m_dialogParent;
};
