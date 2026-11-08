#include "TextSearch.h"

#include <algorithm>

namespace scribe::core
{

namespace
{

Qt::CaseSensitivity caseOf(const SearchOptions& options)
{
    return options.matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
}

TextRange rangeOf(qsizetype position, const QString& needle)
{
    return {position, needle.size()};
}

} // namespace

std::optional<TextRange> findNext(const QString& text, const QString& needle, qsizetype from,
                                  const SearchOptions& options)
{
    if (needle.isEmpty() || needle.size() > text.size()) {
        return std::nullopt;
    }

    from = std::clamp<qsizetype>(from, 0, text.size());
    qsizetype index = text.indexOf(needle, from, caseOf(options));
    if (index < 0 && options.wrapAround) {
        index = text.indexOf(needle, 0, caseOf(options));
    }
    if (index < 0) {
        return std::nullopt;
    }
    return rangeOf(index, needle);
}

std::optional<TextRange> findPrevious(const QString& text, const QString& needle, qsizetype before,
                                      const SearchOptions& options)
{
    if (needle.isEmpty() || needle.size() > text.size()) {
        return std::nullopt;
    }

    before = std::clamp<qsizetype>(before, 0, text.size());

    // A match must end at or before `before`, so it may start at `before - needle.size()` at
    // the latest. lastIndexOf() takes the latest allowed start.
    const qsizetype latestStart = before - needle.size();
    qsizetype index = -1;
    if (latestStart >= 0) {
        index = text.lastIndexOf(needle, latestStart, caseOf(options));
    }
    if (index < 0 && options.wrapAround) {
        index = text.lastIndexOf(needle, text.size() - needle.size(), caseOf(options));
    }
    if (index < 0) {
        return std::nullopt;
    }
    return rangeOf(index, needle);
}

QList<TextRange> findAll(const QString& text, const QString& needle, const SearchOptions& options,
                         qsizetype maxMatches)
{
    QList<TextRange> matches;
    if (needle.isEmpty() || maxMatches == 0) {
        return matches;
    }

    qsizetype from = 0;
    while (true) {
        const qsizetype index = text.indexOf(needle, from, caseOf(options));
        if (index < 0) {
            break;
        }
        matches.append(rangeOf(index, needle));
        if (maxMatches > 0 && matches.size() >= maxMatches) {
            break;
        }
        from = index + needle.size(); // the next match may not overlap this one
    }
    return matches;
}

ReplaceResult replaceAll(const QString& text, const QString& needle, const QString& replacement,
                         const SearchOptions& options)
{
    const QList<TextRange> matches = findAll(text, needle, options);
    if (matches.isEmpty()) {
        return {text, 0};
    }

    QString result;
    result.reserve(text.size());
    qsizetype copiedUpTo = 0;
    for (const TextRange& match : matches) {
        result.append(QStringView(text).sliced(copiedUpTo, match.position - copiedUpTo));
        result.append(replacement);
        copiedUpTo = match.end();
    }
    result.append(QStringView(text).sliced(copiedUpTo));
    return {result, matches.size()};
}

} // namespace scribe::core
