#include "EditorSearch.h"

#include "EditorWidget.h"

#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>

namespace scribe::editor
{

namespace
{

QTextCursor cursorFor(const EditorWidget& editor, const core::TextRange& range)
{
    QTextCursor cursor(editor.document());
    cursor.setPosition(static_cast<int>(range.position));
    cursor.setPosition(static_cast<int>(range.end()), QTextCursor::KeepAnchor);
    return cursor;
}

} // namespace

void EditorSearch::select(EditorWidget& editor, const core::TextRange& range)
{
    QTextCursor start(editor.document());
    start.setPosition(static_cast<int>(range.position));
    const bool alreadyVisible = editor.viewport()->rect().contains(editor.cursorRect(start));

    editor.setTextCursor(cursorFor(editor, range));
    if (!alreadyVisible) {
        editor.centerCursor();
    }
}

bool EditorSearch::selectionIsMatch(const EditorWidget& editor, const QString& needle,
                                    const core::SearchOptions& options)
{
    const QTextCursor cursor = editor.textCursor();
    if (needle.isEmpty() || !cursor.hasSelection()) {
        return false;
    }

    // Qt represents a line break inside a selection as U+2029.
    QString selected = cursor.selectedText();
    selected.replace(QChar(0x2029), QLatin1Char('\n'));
    const Qt::CaseSensitivity sensitivity =
        options.matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
    return QString::compare(selected, needle, sensitivity) == 0;
}

void EditorSearch::replaceSelection(EditorWidget& editor, const QString& replacement)
{
    QTextCursor cursor = editor.textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    // An edit block keeps this a separate undo step instead of merging with nearby typing.
    cursor.beginEditBlock();
    cursor.insertText(replacement);
    cursor.endEditBlock();
    editor.setTextCursor(cursor);
}

qsizetype EditorSearch::replaceRanges(EditorWidget& editor, const QList<core::TextRange>& ranges,
                                      const QString& replacement)
{
    if (ranges.isEmpty()) {
        return 0;
    }

    // Replacing from the last range to the first keeps every earlier position valid, even when
    // the replacement has a different length. The whole thing is one edit block: one undo step.
    // (Setting the whole text instead would also throw away the undo history and the caret.)
    QTextCursor cursor(editor.document());
    cursor.beginEditBlock();
    for (auto it = ranges.crbegin(); it != ranges.crend(); ++it) {
        cursor.setPosition(static_cast<int>(it->position));
        cursor.setPosition(static_cast<int>(it->end()), QTextCursor::KeepAnchor);
        cursor.insertText(replacement);
    }
    cursor.endEditBlock();

    // The first replacement is not moved by any of the others, so its end is easy to compute.
    QTextCursor caret(editor.document());
    caret.setPosition(static_cast<int>(ranges.first().position + replacement.size()));
    editor.setTextCursor(caret);
    return ranges.size();
}

void EditorSearch::highlight(EditorWidget& editor, const std::optional<core::TextRange>& range)
{
    if (!range) {
        editor.setExtraSelections({});
        return;
    }

    QTextEdit::ExtraSelection mark;
    mark.cursor = cursorFor(editor, *range);
    // The active colors, so the match stands out even when the editor itself is not focused.
    mark.format.setBackground(editor.palette().color(QPalette::Active, QPalette::Highlight));
    mark.format.setForeground(editor.palette().color(QPalette::Active, QPalette::HighlightedText));
    editor.setExtraSelections({mark});
}

} // namespace scribe::editor
