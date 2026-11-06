#include "DocumentController.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QFileInfo>

using scribe::core::DocumentError;
using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;

DocumentController::DocumentController(EditorTabs& tabs, FileDialogs dialogs, QWidget* dialogParent)
    : m_tabs(tabs)
    , m_dialogs(std::move(dialogs))
    , m_dialogParent(dialogParent)
{
    m_tabs.setCloseGuard([this](EditorWidget& editor) { return confirmClose(editor); });
}

DocumentController::~DocumentController()
{
    m_tabs.setCloseGuard({}); // the guard captures `this`
}

void DocumentController::newFile()
{
    m_tabs.addEditor();
}

void DocumentController::openFile()
{
    const QString path = m_dialogs.chooseOpenPath(m_dialogParent);
    if (path.isEmpty()) {
        return;
    }

    const scribe::core::IoResult result = m_tabs.openFile(path);
    if (!result.ok()) {
        showIoError(result, path, *m_tabs.currentEditor());
    }
}

void DocumentController::saveFile()
{
    save(*m_tabs.currentEditor());
}

void DocumentController::saveFileAs()
{
    saveAs(*m_tabs.currentEditor());
}

bool DocumentController::save(EditorWidget& editor)
{
    const QString& path = editor.fileDocument().filePath();
    if (path.isEmpty()) {
        return saveAs(editor);
    }

    const scribe::core::IoResult result = editor.saveFile();
    if (!result.ok()) {
        showIoError(result, path, editor);
        return false;
    }
    return true;
}

bool DocumentController::saveAs(EditorWidget& editor)
{
    const QString path = m_dialogs.chooseSavePath(m_dialogParent, editor.fileDocument().filePath());
    if (path.isEmpty()) {
        return false;
    }

    const scribe::core::IoResult result = editor.saveFileAs(path);
    if (!result.ok()) {
        showIoError(result, path, editor);
        return false;
    }
    return true;
}

bool DocumentController::confirmClose(EditorWidget& editor)
{
    // Without a way to ask, the safe answer is to keep the document.
    const UnsavedChoice choice =
        m_dialogs.confirmDiscard ? m_dialogs.confirmDiscard(m_dialogParent, editor.displayName())
                                 : UnsavedChoice::Cancel;

    switch (choice) {
        case UnsavedChoice::Save:
            return save(editor); // a cancelled or failed save keeps the tab open
        case UnsavedChoice::Discard:
            return true;
        case UnsavedChoice::Cancel:
            break;
    }
    return false;
}

void DocumentController::showIoError(const scribe::core::IoResult& result, const QString& path,
                                     const EditorWidget& editor)
{
    const QString name = QFileInfo(path).fileName();

    QString message;
    switch (result.error) {
        case DocumentError::None:
            return;
        case DocumentError::CannotRead:
            message = tr("Could not open \"%1\".").arg(name);
            break;
        case DocumentError::InvalidEncoding:
            message =
                tr("\"%1\" was not opened because its text could not be read without changing "
                   "it.")
                    .arg(name);
            break;
        case DocumentError::CannotEncode:
            message = tr("\"%1\" was not saved because the text contains characters that the "
                         "%2 encoding cannot store.")
                          .arg(name, scribe::core::encodingName(editor.fileDocument().encoding()));
            break;
        case DocumentError::CannotWrite:
            message = tr("Could not save \"%1\". The file on disk was not changed.").arg(name);
            break;
    }

    if (!result.detail.isEmpty()) {
        message += QStringLiteral("\n\n") + result.detail;
    }
    m_dialogs.showError(m_dialogParent, tr("Scribe"), message);
}
