#pragma once

#include "TextSearch.h"

#include <QString>

#include <optional>

namespace scribe::editor
{

class EditorWidget;

/// What a search does to an editor: select a match, replace text, mark the current match.
/// The search itself (finding matches) is core::TextSearch; this only applies its results.
///
/// Positions are those of core::TextSearch run on `editor.toPlainText()`. They are the same as
/// the QTextDocument's, because the plain text has exactly one character for each line break.
class EditorSearch
{
public:
    /// Selects `range` and scrolls to it if it is off screen (centered), without scrolling when it
    /// is already visible.
    static void select(EditorWidget& editor, const core::TextRange& range);

    /// Whether the editor's selection is exactly one match of `needle` under `options`.
    [[nodiscard]] static bool selectionIsMatch(const EditorWidget& editor, const QString& needle,
                                               const core::SearchOptions& options);

    /// Replaces the selection with `replacement` as one undo step. Does nothing without a
    /// selection. The caret ends up after the replacement.
    static void replaceSelection(EditorWidget& editor, const QString& replacement);

    /// Replaces every range with `replacement` as ONE undo step, so a single Undo brings all of
    /// them back. `ranges` must be sorted, must not overlap, and must come from the editor's
    /// current text. The caret ends up after the first replacement. Returns how many were
    /// replaced.
    static qsizetype replaceRanges(EditorWidget& editor, const QList<core::TextRange>& ranges,
                                   const QString& replacement);

    /// Marks `range` as the current match, visible even while the editor does not have the
    /// keyboard focus (the normal selection is dimmed then). std::nullopt removes the mark.
    static void highlight(EditorWidget& editor, const std::optional<core::TextRange>& range);
};

} // namespace scribe::editor
