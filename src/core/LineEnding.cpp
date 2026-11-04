#include "LineEnding.h"

#include <algorithm>

namespace scribe::core
{

QString lineEndingSequence(LineEnding ending)
{
    switch (ending) {
        case LineEnding::Crlf:
            return QStringLiteral("\r\n");
        case LineEnding::Lf:
            return QStringLiteral("\n");
        case LineEnding::Cr:
            return QStringLiteral("\r");
    }
    return QStringLiteral("\r\n");
}

QString lineEndingName(LineEnding ending)
{
    switch (ending) {
        case LineEnding::Crlf:
            return QStringLiteral("CRLF");
        case LineEnding::Lf:
            return QStringLiteral("LF");
        case LineEnding::Cr:
            return QStringLiteral("CR");
    }
    return QStringLiteral("CRLF");
}

LineEnding detectLineEnding(const QString& text, LineEnding fallback)
{
    qsizetype crlf = 0;
    qsizetype lf = 0;
    qsizetype cr = 0;

    const qsizetype size = text.size();
    for (qsizetype i = 0; i < size; ++i) {
        const QChar c = text.at(i);
        if (c == u'\r') {
            if (i + 1 < size && text.at(i + 1) == u'\n') {
                ++crlf;
                ++i; // the LF belongs to this CRLF, do not count it again
            } else {
                ++cr;
            }
        } else if (c == u'\n') {
            ++lf;
        }
    }

    const qsizetype most = std::max({crlf, lf, cr});
    if (most == 0) {
        return fallback;
    }

    const int leaders = (crlf == most ? 1 : 0) + (lf == most ? 1 : 0) + (cr == most ? 1 : 0);
    if (leaders > 1) {
        return fallback;
    }

    if (crlf == most) {
        return LineEnding::Crlf;
    }
    return lf == most ? LineEnding::Lf : LineEnding::Cr;
}

QString convertLineEndings(const QString& text, LineEnding target)
{
    const QString newline = lineEndingSequence(target);

    QString result;
    result.reserve(text.size());

    const qsizetype size = text.size();
    for (qsizetype i = 0; i < size; ++i) {
        const QChar c = text.at(i);
        if (c == u'\r') {
            if (i + 1 < size && text.at(i + 1) == u'\n') {
                ++i;
            }
            result += newline;
        } else if (c == u'\n') {
            result += newline;
        } else {
            result += c;
        }
    }
    return result;
}

} // namespace scribe::core
