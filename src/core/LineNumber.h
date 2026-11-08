#pragma once

#include <QString>

#include <optional>

namespace scribe::core
{

/// Reads a line number typed by the user. Returns it only if it is a whole number from 1 to
/// `lineCount` (surrounding spaces and leading zeros are fine). Anything else is out of range or
/// not a number and gives std::nullopt: empty text, 0, a number past the last line, a sign,
/// decimals, letters, or a number too large to be a line at all.
[[nodiscard]] std::optional<int> parseLineNumber(const QString& text, int lineCount);

} // namespace scribe::core
