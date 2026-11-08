#pragma once

#include <QList>
#include <QString>

#include <optional>

namespace scribe::core
{

struct SearchOptions
{
    bool matchCase = false;
    bool wrapAround = true; ///< Used by findNext() and findPrevious(); ignored by the others.
};

/// A piece of text: `length` UTF-16 code units starting at `position`.
struct TextRange
{
    qsizetype position = 0;
    qsizetype length = 0;

    [[nodiscard]] qsizetype end() const { return position + length; }

    friend bool operator==(const TextRange&, const TextRange&) = default;
};

/// Searching plain text. Everything here works on a QString and knows nothing about widgets, so
/// it can be tested thoroughly and reused (for example to search files later).
///
/// Rules shared by all functions:
/// - An empty `needle` never matches.
/// - Matches never overlap: "aa" occurs once in "aaa" and twice in "aaaa".
/// - Without matchCase, letters are compared with Qt's simple case folding (so "É" matches "é",
///   but special cases such as German "ß" versus "SS" are not matched).
/// - Positions and lengths count UTF-16 code units, like QString and QTextDocument.

/// The first match that starts at or after `from`. When there is none and `wrapAround` is set,
/// the first match in the whole text (which then starts before `from`). `from` is clamped to the
/// text.
[[nodiscard]] std::optional<TextRange> findNext(const QString& text, const QString& needle,
                                                qsizetype from, const SearchOptions& options);

/// The last match that ends at or before `before`. When there is none and `wrapAround` is set,
/// the last match in the whole text. Searching backwards from the start of a selection, as
/// findPrevious(text, needle, selection.position, ...) does, never returns a match that overlaps
/// the selection.
[[nodiscard]] std::optional<TextRange> findPrevious(const QString& text, const QString& needle,
                                                    qsizetype before, const SearchOptions& options);

/// Every match, left to right. `maxMatches` (if not negative) stops the search after that many,
/// which lets a caller count "up to N" on a huge text without scanning all of it.
[[nodiscard]] QList<TextRange> findAll(const QString& text, const QString& needle,
                                       const SearchOptions& options, qsizetype maxMatches = -1);

struct ReplaceResult
{
    QString text;
    qsizetype count = 0;
};

/// `text` with every match of `needle` replaced by `replacement`, taken literally. The result is
/// not searched again, so a replacement that contains the needle does not loop.
[[nodiscard]] ReplaceResult replaceAll(const QString& text, const QString& needle,
                                       const QString& replacement, const SearchOptions& options);

} // namespace scribe::core
