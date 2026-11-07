#include "LineNumber.h"

namespace scribe::core
{

std::optional<int> parseLineNumber(const QString& text, int lineCount)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || lineCount < 1) {
        return std::nullopt;
    }

    // Only ASCII digits: toLongLong() would also accept a sign, and QChar::isDigit() accepts
    // digits of other scripts, which this field is not meant to take.
    for (const QChar c : trimmed) {
        if (c < u'0' || c > u'9') {
            return std::nullopt;
        }
    }

    bool ok = false;
    const qlonglong value = trimmed.toLongLong(&ok);
    if (!ok || value < 1 || value > lineCount) {
        return std::nullopt; // not parsable (far too many digits), zero, or past the last line
    }
    return static_cast<int>(value);
}

} // namespace scribe::core
