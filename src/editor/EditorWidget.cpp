#include "EditorWidget.h"

#include <QFileInfo>
#include <QPainter>
#include <QScrollBar>
#include <QTextCursor>
#include <QWheelEvent>

#include <algorithm>

namespace scribe::editor
{

namespace
{
// Sizes from the Figma main window: the gutter is 56 px plus the 4 px that the text margin grows
// by, numbers sit 16 px from its right edge, and the text starts 8 px after the gutter.
constexpr int kMinGutterWidth = 60;
constexpr int kNumberPaddingLeft = 8;
constexpr int kNumberPaddingRight = 16;
constexpr int kTextMargin = 8;
constexpr qreal kNumberFontScale = 13.0 / 15.0; // 13 px numbers beside 15 px text
} // namespace

/// The line numbers. All the drawing logic is in EditorWidget::paintGutter(), which knows the
/// blocks and the scroll position.
class EditorWidget::Gutter : public QWidget
{
public:
    explicit Gutter(EditorWidget& editor)
        : QWidget(&editor)
        , m_editor(editor)
    {}

protected:
    void paintEvent(QPaintEvent* event) override { m_editor.paintGutter(event); }

private:
    EditorWidget& m_editor;
};

EditorWidget::EditorWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    connect(document(), &QTextDocument::modificationChanged, this, &EditorWidget::stateChanged);

    // The area below the text is the editor's own colour (see paintEvent), not the window's.
    setBackgroundVisible(false);
    viewport()->setAutoFillBackground(false);
    document()->setDocumentMargin(kTextMargin);
    // Changing the margin counts as an edit; a new editor is clean and has nothing to undo.
    document()->clearUndoRedoStacks();
    document()->setModified(false);

    m_gutter = new Gutter(*this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &EditorWidget::updateGutterWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &EditorWidget::updateGutter);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        // Both the old and the new line change, so repaint rather than work out which.
        viewport()->update();
        m_gutter->update();
    });
    updateGutterWidth();
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
    emit fileUsed(path);
    return result;
}

core::IoResult EditorWidget::reload()
{
    const QString path = m_fileDocument.filePath();
    if (path.isEmpty()) {
        return {core::DocumentError::CannotRead, QStringLiteral("no file path")};
    }

    core::Document loaded;
    const core::IoResult result = loaded.load(path);
    if (!result.ok()) {
        return result;
    }

    const QTextCursor before = textCursor();
    const int line = before.blockNumber();
    const int column = before.positionInBlock();
    const int scroll = verticalScrollBar()->value();

    m_fileDocument = std::move(loaded);
    setPlainText(m_fileDocument.backend().text());
    document()->setModified(false);

    const QTextBlock block = document()->findBlockByNumber(std::min(line, lineCount() - 1));
    QTextCursor after(block);
    after.setPosition(block.position() + std::min(column, block.length() - 1));
    setTextCursor(after);
    verticalScrollBar()->setValue(scroll);

    emit stateChanged();
    return result;
}

void EditorWidget::acknowledgeDiskChange()
{
    m_fileDocument.refreshDiskStamp();
}

void EditorWidget::keepAsUnsavedCopy()
{
    m_fileDocument.refreshDiskStamp();
    document()->setModified(true);
    emit stateChanged();
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
        emit fileUsed(path);
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

int EditorWidget::gutterWidth() const
{
    const int digits = std::max<qsizetype>(2, QString::number(blockCount()).size());
    const int numbers = fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return std::max(kMinGutterWidth, kNumberPaddingLeft + numbers + kNumberPaddingRight);
}

void EditorWidget::updateGutterWidth()
{
    setViewportMargins(gutterWidth(), 0, 0, 0);
    const QRect area = contentsRect();
    m_gutter->setGeometry(area.left(), area.top(), gutterWidth(), area.height());
}

void EditorWidget::updateGutter(const QRect& rect, int dy)
{
    if (dy != 0) {
        m_gutter->scroll(0, dy);
    } else {
        m_gutter->update(0, rect.y(), m_gutter->width(), rect.height());
    }
}

QRectF EditorWidget::textRectOf(const QTextBlock& block) const
{
    // The block's own rectangle also holds the document margin above the first line and below the
    // last one. The caret rectangles of its first and last position are exactly the lines, and a
    // wrapped line (one block, several rows) is covered from its first row to its last.
    QTextCursor first(block);
    QTextCursor last(block);
    last.movePosition(QTextCursor::EndOfBlock);
    const QRect top = cursorRect(first);
    const QRect bottom = cursorRect(last);
    return QRectF(top.left(), top.top(), 0, bottom.bottom() - top.top() + 1);
}

int EditorWidget::firstRowHeight(const QTextBlock& block) const
{
    return cursorRect(QTextCursor(block)).height();
}

void EditorWidget::paintEvent(QPaintEvent* event)
{
    {
        QPainter painter(viewport());
        painter.fillRect(event->rect(), palette().color(QPalette::Base));

        QRectF band = textRectOf(textCursor().block());
        band.setLeft(0);
        band.setRight(viewport()->width());
        painter.fillRect(band, palette().color(QPalette::AlternateBase));
    }
    QPlainTextEdit::paintEvent(event);
}

void EditorWidget::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    updateGutterWidth();
}

void EditorWidget::changeEvent(QEvent* event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        updateGutterWidth(); // the digits are wider or narrower
    }
}

void EditorWidget::paintGutter(QPaintEvent* event)
{
    QPainter painter(m_gutter);
    const QPalette& colors = palette();
    painter.fillRect(event->rect(), colors.color(QPalette::Base));

    QFont numberFont = font();
    if (numberFont.pointSizeF() > 0) {
        numberFont.setPointSizeF(numberFont.pointSizeF() * kNumberFontScale);
    }
    painter.setFont(numberFont);

    const int currentBlock = textCursor().blockNumber();
    const int textWidth = m_gutter->width() - kNumberPaddingRight;

    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        const QRectF lines = textRectOf(block);
        if (lines.top() > event->rect().bottom()) {
            break;
        }
        if (block.isVisible() && lines.bottom() >= event->rect().top()) {
            const bool current = block.blockNumber() == currentBlock;
            if (current) {
                painter.fillRect(QRectF(0, lines.top(), m_gutter->width(), lines.height()),
                                 colors.color(QPalette::AlternateBase));
            }
            // The number goes beside the block's first line, even when the block wraps.
            painter.setPen(colors.color(current ? QPalette::Text : QPalette::PlaceholderText));
            painter.drawText(QRectF(0, lines.top(), textWidth, firstRowHeight(block)),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(block.blockNumber() + 1));
        }
        block = block.next();
    }
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
