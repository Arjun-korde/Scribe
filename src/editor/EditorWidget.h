#pragma once

#include "Document.h"

#include <QPlainTextEdit>

namespace scribe::editor
{

/// A text editor bound to one core::Document (file path, encoding, line ending, modified state).
///
/// While the user types, the QPlainTextEdit is the source of truth, so undo and redo work as
/// usual. The text is copied into the Document only when saving, and from the Document when
/// opening a file.
class EditorWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit EditorWidget(QWidget* parent = nullptr);

    /// The file-level state. Its backend text is only up to date right after load and save.
    const core::Document& fileDocument() const { return m_fileDocument; }

    /// True when the text changed since the last load or save, or when the encoding or line ending
    /// was changed.
    bool isModified() const;

    /// The file name for window titles and tabs, or "Untitled" for a document without a file.
    QString displayName() const;

    /// Replaces the content with an empty, unsaved document.
    void newFile();

    /// On failure the editor keeps its current content.
    core::IoResult openFile(const QString& path);

    /// Saves to the current path. A document without a path fails with CannotWrite.
    core::IoResult saveFile();

    /// Saves to `path`, which becomes the document's path on success.
    core::IoResult saveFileAs(const QString& path);

    void setEncoding(const core::EncodingInfo& encoding);
    void setLineEnding(core::LineEnding lineEnding);

signals:
    /// Emitted when the path, the modified state, the encoding or the line ending changes.
    void stateChanged();

    /// Ctrl+wheel was used: zoom by `steps` notches (positive zooms in). The editor does not zoom
    /// itself, so that all tabs share one zoom level.
    void zoomRequested(int steps);

protected:
    /// Turns Ctrl+wheel into zoomRequested(). QPlainTextEdit would otherwise change the font size
    /// on its own, in points, outside the shared zoom percentage.
    void wheelEvent(QWheelEvent* event) override;

private:
    /// Copies the widget text into the Document when the two differ. Comparing the text, rather
    /// than trusting modified flags, covers every case: a failed save leaves the Document holding
    /// text the widget may since have undone, QPlainTextEdit::setPlainText() resets the widget's
    /// flag, and a failed save of an unchanged file must not make it look modified (the Document
    /// marks itself modified on every setText).
    void syncTextToDocument();
    void markSaved();

    core::Document m_fileDocument;
    int m_wheelRemainder =
        0; ///< Wheel movement not yet worth a full notch (touchpads send tiny ones).
};

} // namespace scribe::editor
