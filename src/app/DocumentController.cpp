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
    if (!path.isEmpty()) {
        openFiles({path});
    }
}

void DocumentController::openFiles(const QStringList& paths)
{
    for (const QString& path : paths) {
        const scribe::core::IoResult result = m_tabs.openFile(path);
        if (!result.ok()) {
            showIoError(result, path, *m_tabs.currentEditor());
        }
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

bool DocumentController::setEncoding(const scribe::core::EncodingInfo& encoding)
{
    EditorWidget& editor = *m_tabs.currentEditor();
    if (editor.fileDocument().encoding() == encoding) {
        return true;
    }

    // Line breaks never matter for this check (they exist in every encoding), so the widget's
    // text can be tested as it is.
    if (!scribe::core::encode(editor.toPlainText(), encoding)) {
        m_dialogs.showError(
            m_dialogParent, tr("Scribe"),
            tr("The encoding was not changed to %1 because the text contains characters that it "
               "cannot store.")
                .arg(scribe::core::encodingName(encoding)));
        return false;
    }

    editor.setEncoding(encoding);
    return true;
}

void DocumentController::setLineEnding(scribe::core::LineEnding lineEnding)
{
    m_tabs.currentEditor()->setLineEnding(lineEnding);
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
