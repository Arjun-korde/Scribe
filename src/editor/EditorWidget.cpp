#include "EditorWidget.h"

#include <QFileInfo>

namespace scribe::editor
{

EditorWidget::EditorWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    connect(document(), &QTextDocument::modificationChanged, this, &EditorWidget::stateChanged);
}

bool EditorWidget::isModified() const
{
    return document()->isModified() || m_fileDocument.isModified();
}

QString EditorWidget::displayName() const
{
    const QString& path = m_fileDocument.filePath();
    return path.isEmpty() ? tr("Untitled") : QFileInfo(path).fileName();
}

void EditorWidget::newFile()
{
    m_fileDocument = core::Document();
    clear();
    document()->setModified(false);
    emit stateChanged();
}

core::IoResult EditorWidget::openFile(const QString& path)
{
    core::Document loaded;
    const core::IoResult result = loaded.load(path);
    if (!result.ok()) {
        return result;
    }

    m_fileDocument = std::move(loaded);
    setPlainText(m_fileDocument.backend().text());
    document()->setModified(false);
    emit stateChanged();
    return result;
}

core::IoResult EditorWidget::saveFile()
{
    syncTextToDocument();
    const core::IoResult result = m_fileDocument.save();
    if (result.ok()) {
        markSaved();
    }
    return result;
}

core::IoResult EditorWidget::saveFileAs(const QString& path)
{
    syncTextToDocument();
    const core::IoResult result = m_fileDocument.saveAs(path);
    if (result.ok()) {
        markSaved();
    }
    return result;
}

void EditorWidget::setEncoding(const core::EncodingInfo& encoding)
{
    m_fileDocument.setEncoding(encoding);
    emit stateChanged();
}

void EditorWidget::setLineEnding(core::LineEnding lineEnding)
{
    m_fileDocument.setLineEnding(lineEnding);
    emit stateChanged();
}

void EditorWidget::syncTextToDocument()
{
    const QString text = toPlainText();
    if (m_fileDocument.backend().text() != text) {
        m_fileDocument.setText(text);
    }
}

void EditorWidget::markSaved()
{
    document()->setModified(false);
    emit stateChanged();
}

} // namespace scribe::editor
