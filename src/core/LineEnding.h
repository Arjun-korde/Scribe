#pragma once

#include <QString>

namespace scribe::core
{

enum class LineEnding
{
    Crlf,
    Lf,
    Cr,
};

/// The characters that make up one line break of the given style.
QString lineEndingSequence(LineEnding ending);

/// Display name used in the status bar and menus: "CRLF", "LF" or "CR".
QString lineEndingName(LineEnding ending);

/// Returns the most frequent line ending in `text`. When the text has no line breaks, or two
/// styles are tied, `fallback` is returned (a tie keeps the user's default instead of guessing).
LineEnding detectLineEnding(const QString& text, LineEnding fallback = LineEnding::Crlf);

/// Rewrites every line break in `text` (CRLF, LF or lone CR, including mixed) to `target`.
QString convertLineEndings(const QString& text, LineEnding target);

} // namespace scribe::core
