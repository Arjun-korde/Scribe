#include "LineNumber.h"

#include <gtest/gtest.h>

using scribe::core::parseLineNumber;

TEST(LineNumber, AcceptsNumbersInsideTheDocument)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("1"), 300), 1);
    EXPECT_EQ(parseLineNumber(QStringLiteral("42"), 300), 42);
    EXPECT_EQ(parseLineNumber(QStringLiteral("300"), 300), 300); // the last line itself
}

TEST(LineNumber, RejectsNumbersPastTheLastLine)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("301"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("310"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("99999"), 300), std::nullopt);
}

TEST(LineNumber, RejectsZeroAndNegativeNumbers)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("0"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("-1"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("-0"), 300), std::nullopt);
}

TEST(LineNumber, RejectsEmptyAndBlankText)
{
    EXPECT_EQ(parseLineNumber(QString(), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral(""), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("   "), 300), std::nullopt);
}

TEST(LineNumber, RejectsAnythingThatIsNotJustDigits)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("abc"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("12a"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("1 2"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("+5"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("5.0"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("1e2"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("0x10"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("1,000"), 3000), std::nullopt);
}

TEST(LineNumber, IgnoresSurroundingSpacesAndLeadingZeros)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("  7  "), 300), 7);
    EXPECT_EQ(parseLineNumber(QStringLiteral("007"), 300), 7);
    EXPECT_EQ(parseLineNumber(QStringLiteral("000000000000000000000005"), 300), 5);
}

TEST(LineNumber, OnlyTakesAsciiDigits)
{
    // Arabic-Indic digits and full-width digits look like numbers but are not accepted.
    EXPECT_EQ(parseLineNumber(QString::fromUtf8("\xD9\xA5"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QString::fromUtf8("\xEF\xBC\x95"), 300), std::nullopt);
}

TEST(LineNumber, HugeNumbersAreRejectedWithoutOverflowing)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("2147483648"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("9223372036854775807"), 300), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("99999999999999999999999999"), 300), std::nullopt);
    // Even with a (hypothetically) enormous document the arithmetic stays correct.
    EXPECT_EQ(parseLineNumber(QStringLiteral("2147483647"), 2147483647), 2147483647);
}

TEST(LineNumber, AOneLineDocumentOnlyHasLineOne)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("1"), 1), 1);
    EXPECT_EQ(parseLineNumber(QStringLiteral("2"), 1), std::nullopt);
}

TEST(LineNumber, NoLinesMeansNothingIsValid)
{
    EXPECT_EQ(parseLineNumber(QStringLiteral("1"), 0), std::nullopt);
    EXPECT_EQ(parseLineNumber(QStringLiteral("1"), -5), std::nullopt);
}
