#include "LineEnding.h"

#include <gtest/gtest.h>

using scribe::core::convertLineEndings;
using scribe::core::detectLineEnding;
using scribe::core::LineEnding;

TEST(LineEndingDetect, EmptyTextReturnsFallback)
{
    EXPECT_EQ(detectLineEnding(QString()), LineEnding::Crlf);
    EXPECT_EQ(detectLineEnding(QString(), LineEnding::Lf), LineEnding::Lf);
}

TEST(LineEndingDetect, NoLineBreaksReturnsFallback)
{
    EXPECT_EQ(detectLineEnding(QStringLiteral("hello"), LineEnding::Lf), LineEnding::Lf);
}

TEST(LineEndingDetect, DetectsEachStyle)
{
    EXPECT_EQ(detectLineEnding(QStringLiteral("a\r\nb\r\nc")), LineEnding::Crlf);
    EXPECT_EQ(detectLineEnding(QStringLiteral("a\nb\nc"), LineEnding::Crlf), LineEnding::Lf);
    EXPECT_EQ(detectLineEnding(QStringLiteral("a\rb\rc"), LineEnding::Crlf), LineEnding::Cr);
}

TEST(LineEndingDetect, CrlfIsNotCountedAsCrPlusLf)
{
    // One CRLF must not tie with a lone LF.
    EXPECT_EQ(detectLineEnding(QStringLiteral("a\r\nb\r\nc\nd"), LineEnding::Lf), LineEnding::Crlf);
}

TEST(LineEndingDetect, MixedPicksMostFrequent)
{
    EXPECT_EQ(detectLineEnding(QStringLiteral("a\nb\nc\r\nd"), LineEnding::Crlf), LineEnding::Lf);
}

TEST(LineEndingDetect, TieReturnsFallback)
{
    EXPECT_EQ(detectLineEnding(QStringLiteral("a\nb\r\nc"), LineEnding::Cr), LineEnding::Cr);
}

TEST(LineEndingConvert, ConvertsEveryStyleToEveryStyle)
{
    const QString lf = QStringLiteral("a\nb\n");
    const QString crlf = QStringLiteral("a\r\nb\r\n");
    const QString cr = QStringLiteral("a\rb\r");

    for (const QString& source : {lf, crlf, cr}) {
        EXPECT_EQ(convertLineEndings(source, LineEnding::Lf), lf);
        EXPECT_EQ(convertLineEndings(source, LineEnding::Crlf), crlf);
        EXPECT_EQ(convertLineEndings(source, LineEnding::Cr), cr);
    }
}

TEST(LineEndingConvert, NormalizesMixedText)
{
    EXPECT_EQ(convertLineEndings(QStringLiteral("a\r\nb\nc\rd"), LineEnding::Lf),
              QStringLiteral("a\nb\nc\nd"));
}

TEST(LineEndingConvert, KeepsBlankLinesAndOtherCharacters)
{
    EXPECT_EQ(convertLineEndings(QStringLiteral("\n\nxé\U0001F600\n"), LineEnding::Crlf),
              QStringLiteral("\r\n\r\nxé\U0001F600\r\n"));
    EXPECT_EQ(convertLineEndings(QString(), LineEnding::Lf), QString());
}

TEST(LineEndingConvert, ConsecutiveCrBeforeLfAreSeparateBreaks)
{
    // "\r\r\n" is a lone CR followed by CRLF: two breaks, not one.
    EXPECT_EQ(convertLineEndings(QStringLiteral("a\r\r\nb"), LineEnding::Lf),
              QStringLiteral("a\n\nb"));
}
