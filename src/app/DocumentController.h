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

    /// Opens each file in order, with the same rules as File > Open: a file that is already open
    /// is switched to, an untouched empty tab is reused, anything else gets a new tab. The last
    /// file to open (or be switched to) is left active. A file that fails shows its own error
    /// dialog; the other files are still opened. Never asks about unsaved changes, since opening
    /// only adds tabs. Used by drag and drop.
    void openFiles(const QStringList& paths);

    void saveFile();
    void saveFileAs();

    /// Changes how the active document will be written by the next save (the text is not
    /// re-read). Refused, with an explanation and no change, if the text has characters the new
    /// encoding cannot store, so a later save cannot fail for that reason. Returns whether the
    /// encoding is now `encoding`.
    bool setEncoding(const scribe::core::EncodingInfo& encoding);

    /// Changes the line ending the next save will write. Always possible.
    void setLineEnding(scribe::core::LineEnding lineEnding);

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
