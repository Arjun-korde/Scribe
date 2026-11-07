#include "EditorWidget.h"

#include <QFileInfo>
#include <QTextBlock>
#include <QWheelEvent>

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

bool EditorWidget::goToLine(int line)
{
    if (line < 1 || line > lineCount()) {
        return false;
    }

    const QTextCursor target(document()->findBlockByNumber(line - 1)); // at the block's start
    const bool alreadyVisible = viewport()->rect().contains(cursorRect(target));

    setTextCursor(target);
    if (!alreadyVisible) {
        centerCursor(); // setTextCursor only scrolls far enough to show the line at an edge
    }
    setFocus();
    return true;
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

void EditorWidget::wheelEvent(QWheelEvent* event)
{
    if (!(event->modifiers() & Qt::ControlModifier)) {
        QPlainTextEdit::wheelEvent(event);
        return;
    }

    // One notch of a mouse wheel is 120 units. Touchpads send many small movements, so collect
    // them until they add up to a notch.
    constexpr int kUnitsPerNotch = 120;
    m_wheelRemainder += event->angleDelta().y();
    const int steps = m_wheelRemainder / kUnitsPerNotch;
    m_wheelRemainder -= steps * kUnitsPerNotch;

    if (steps != 0) {
        emit zoomRequested(steps);
    }
    event->accept();
}

} // namespace scribe::editor
