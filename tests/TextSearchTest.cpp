#include "TextSearch.h"

#include <gtest/gtest.h>

using scribe::core::findAll;
using scribe::core::findNext;
using scribe::core::findPrevious;
using scribe::core::replaceAll;
using scribe::core::SearchOptions;
using scribe::core::TextRange;

namespace
{

QString s(const char* text)
{
    return QString::fromUtf8(text);
}

SearchOptions options(bool matchCase = false, bool wrap = true)
{
    SearchOptions result;
    result.matchCase = matchCase;
    result.wrapAround = wrap;
    return result;
}

} // namespace

// ---- findNext -------------------------------------------------------------------------------

TEST(TextSearchNext, FindsTheFirstMatchAtOrAfterThePosition)
{
    const QString text = s("one two one two");
    EXPECT_EQ(findNext(text, s("two"), 0, options()), (TextRange{4, 3}));
    EXPECT_EQ(findNext(text, s("two"), 4, options()), (TextRange{4, 3})); // exactly at `from`
    EXPECT_EQ(findNext(text, s("two"), 5, options()), (TextRange{12, 3}));
}

TEST(TextSearchNext, WrapsToTheFirstMatchWhenNothingFollows)
{
    const QString text = s("one two one two");
    EXPECT_EQ(findNext(text, s("one"), 13, options(false, true)), (TextRange{0, 3}));
}

TEST(TextSearchNext, DoesNotWrapWhenWrapAroundIsOff)
{
    const QString text = s("one two one two");
    EXPECT_EQ(findNext(text, s("one"), 13, options(false, false)), std::nullopt);
    EXPECT_EQ(findNext(text, s("one"), 1, options(false, false)), (TextRange{8, 3}));
}

TEST(TextSearchNext, TheOnlyMatchIsFoundAgainAfterWrapping)
{
    const QString text = s("a needle here");
    EXPECT_EQ(findNext(text, s("needle"), 9, options()), (TextRange{2, 6}));
}

TEST(TextSearchNext, NoMatchAnywhereIsNothing)
{
    EXPECT_EQ(findNext(s("abc"), s("x"), 0, options()), std::nullopt);
}

TEST(TextSearchNext, AnEmptyNeedleNeverMatches)
{
    EXPECT_EQ(findNext(s("abc"), QString(), 0, options()), std::nullopt);
    EXPECT_EQ(findNext(QString(), QString(), 0, options()), std::nullopt);
}

TEST(TextSearchNext, AnEmptyTextOrALongerNeedleNeverMatches)
{
    EXPECT_EQ(findNext(QString(), s("a"), 0, options()), std::nullopt);
    EXPECT_EQ(findNext(s("ab"), s("abc"), 0, options()), std::nullopt);
}

TEST(TextSearchNext, ThePositionIsClampedToTheText)
{
    EXPECT_EQ(findNext(s("abc abc"), s("abc"), -50, options()), (TextRange{0, 3}));
    EXPECT_EQ(findNext(s("abc abc"), s("abc"), 9999, options(false, true)), (TextRange{0, 3}));
    EXPECT_EQ(findNext(s("abc abc"), s("abc"), 9999, options(false, false)), std::nullopt);
}

TEST(TextSearchNext, FindsAMatchAtTheVeryEnd)
{
    EXPECT_EQ(findNext(s("xxabc"), s("abc"), 2, options()), (TextRange{2, 3}));
    EXPECT_EQ(findNext(s("xxabc"), s("c"), 4, options()), (TextRange{4, 1}));
}

TEST(TextSearchNext, IgnoresCaseUnlessAskedNotTo)
{
    EXPECT_EQ(findNext(s("Hello hello HELLO"), s("hello"), 0, options(false)), (TextRange{0, 5}));
    EXPECT_EQ(findNext(s("Hello hello HELLO"), s("hello"), 0, options(true)), (TextRange{6, 5}));
    EXPECT_EQ(findNext(s("Hello hello HELLO"), s("HELLO"), 0, options(true)), (TextRange{12, 5}));
    EXPECT_EQ(findNext(s("Hello"), s("hello"), 0, options(true, false)), std::nullopt);
}

TEST(TextSearchNext, ComparesAccentedLettersWithoutCaseToo)
{
    EXPECT_EQ(findNext(s("CAF\xC3\x89"), s("caf\xC3\xA9"), 0, options(false)), (TextRange{0, 4}));
    EXPECT_EQ(findNext(s("CAF\xC3\x89"), s("caf\xC3\xA9"), 0, options(true)), std::nullopt);
}

TEST(TextSearchNext, CountsUtf16CodeUnitsForEmoji)
{
    // The emoji is a surrogate pair: two code units, so "b" is at position 3, not 2.
    const QString text = s("a\xF0\x9F\x98\x80"
                           "b");
    EXPECT_EQ(findNext(text, s("b"), 0, options()), (TextRange{3, 1}));
    EXPECT_EQ(findNext(text, s("\xF0\x9F\x98\x80"), 0, options()), (TextRange{1, 2}));
}

TEST(TextSearchNext, MatchesLineBreaksLikeAnyOtherCharacter)
{
    const QString text = s("one\ntwo\nthree");
    EXPECT_EQ(findNext(text, s("e\nt"), 0, options()), (TextRange{2, 3}));
}

TEST(TextSearchNext, SpecialCharactersAreLiteral)
{
    const QString text = s("a.b*c(d)[e]^$\\");
    EXPECT_EQ(findNext(text, s("."), 0, options()), (TextRange{1, 1}));
    EXPECT_EQ(findNext(text, s("*"), 0, options()), (TextRange{3, 1}));
    EXPECT_EQ(findNext(text, s("(d)"), 0, options()), (TextRange{5, 3}));
    EXPECT_EQ(findNext(text, s("[e]"), 0, options()), (TextRange{8, 3}));
    EXPECT_EQ(findNext(text, s("^$\\"), 0, options()), (TextRange{11, 3}));
}

// ---- findPrevious ---------------------------------------------------------------------------

TEST(TextSearchPrevious, FindsTheLastMatchThatEndsAtOrBeforeThePosition)
{
    const QString text = s("one two one two");
    EXPECT_EQ(findPrevious(text, s("two"), 15, options()), (TextRange{12, 3}));
    EXPECT_EQ(findPrevious(text, s("two"), 12, options()), (TextRange{4, 3}));
    EXPECT_EQ(findPrevious(text, s("two"), 7, options()), (TextRange{4, 3})); // ends exactly at 7
    EXPECT_EQ(findPrevious(text, s("two"), 6, options(false, false)), std::nullopt);
}

TEST(TextSearchPrevious, NeverReturnsAMatchThatOverlapsTheStartPosition)
{
    // "aa" in "aaaa" occurs at 0 and 2. Going back from a selection that starts at 2 must give 0,
    // not the overlapping occurrence at 1.
    EXPECT_EQ(findPrevious(s("aaaa"), s("aa"), 2, options(false, false)), (TextRange{0, 2}));
    // The match at 1 occupies 1..2 and ends exactly at 3, so it does not overlap a start of 3.
    EXPECT_EQ(findPrevious(s("aaaa"), s("aa"), 3, options(false, false)), (TextRange{1, 2}));
}

TEST(TextSearchPrevious, WrapsToTheLastMatchWhenNothingPrecedes)
{
    const QString text = s("one two one two");
    EXPECT_EQ(findPrevious(text, s("two"), 3, options(false, true)), (TextRange{12, 3}));
}

TEST(TextSearchPrevious, DoesNotWrapWhenWrapAroundIsOff)
{
    EXPECT_EQ(findPrevious(s("one two"), s("two"), 3, options(false, false)), std::nullopt);
}

TEST(TextSearchPrevious, FromTheStartOnlyWrapsOrFindsNothing)
{
    EXPECT_EQ(findPrevious(s("abc"), s("a"), 0, options(false, true)), (TextRange{0, 1}));
    EXPECT_EQ(findPrevious(s("abc"), s("a"), 0, options(false, false)), std::nullopt);
}

TEST(TextSearchPrevious, ThePositionIsClampedToTheText)
{
    EXPECT_EQ(findPrevious(s("abc abc"), s("abc"), 9999, options()), (TextRange{4, 3}));
    EXPECT_EQ(findPrevious(s("abc abc"), s("abc"), -9, options(false, false)), std::nullopt);
}

TEST(TextSearchPrevious, HonoursMatchCase)
{
    const QString text = s("Hello hello");
    EXPECT_EQ(findPrevious(text, s("hello"), 11, options(false)), (TextRange{6, 5}));
    EXPECT_EQ(findPrevious(text, s("Hello"), 11, options(true)), (TextRange{0, 5}));
    EXPECT_EQ(findPrevious(text, s("HELLO"), 11, options(true, false)), std::nullopt);
}

TEST(TextSearchPrevious, EmptyOrOversizedNeedlesNeverMatch)
{
    EXPECT_EQ(findPrevious(s("abc"), QString(), 3, options()), std::nullopt);
    EXPECT_EQ(findPrevious(s("ab"), s("abc"), 2, options()), std::nullopt);
    EXPECT_EQ(findPrevious(QString(), s("a"), 0, options()), std::nullopt);
}

// ---- findAll --------------------------------------------------------------------------------

TEST(TextSearchAll, ListsMatchesLeftToRight)
{
    EXPECT_EQ(findAll(s("a-b-a-b"), s("a"), options()), (QList<TextRange>{{0, 1}, {4, 1}}));
    EXPECT_EQ(findAll(s("abcabc"), s("bc"), options()), (QList<TextRange>{{1, 2}, {4, 2}}));
}

TEST(TextSearchAll, MatchesDoNotOverlap)
{
    EXPECT_EQ(findAll(s("aaa"), s("aa"), options()), (QList<TextRange>{{0, 2}}));
    EXPECT_EQ(findAll(s("aaaa"), s("aa"), options()), (QList<TextRange>{{0, 2}, {2, 2}}));
    EXPECT_EQ(findAll(s("ababab"), s("aba"), options()), (QList<TextRange>{{0, 3}}));
}

TEST(TextSearchAll, NothingFoundGivesAnEmptyList)
{
    EXPECT_TRUE(findAll(s("abc"), s("x"), options()).isEmpty());
    EXPECT_TRUE(findAll(s("abc"), QString(), options()).isEmpty());
    EXPECT_TRUE(findAll(QString(), s("a"), options()).isEmpty());
}

TEST(TextSearchAll, MatchCaseDecidesWhatCounts)
{
    EXPECT_EQ(findAll(s("Aa aA AA aa"), s("aa"), options(false)).size(), 4);
    EXPECT_EQ(findAll(s("Aa aA AA aa"), s("aa"), options(true)).size(), 1);
}

TEST(TextSearchAll, StopsAfterMaxMatches)
{
    const QString text = s("x x x x x x x x x x");
    EXPECT_EQ(findAll(text, s("x"), options(), 3).size(), 3);
    EXPECT_EQ(findAll(text, s("x"), options(), 100).size(), 10);
    EXPECT_EQ(findAll(text, s("x"), options(), -1).size(), 10);
    EXPECT_TRUE(findAll(text, s("x"), options(), 0).isEmpty());
}

TEST(TextSearchAll, IgnoresWrapAround)
{
    EXPECT_EQ(findAll(s("a a"), s("a"), options(false, false)).size(), 2);
}

TEST(TextSearchAll, HandlesAHugeTextWithManyMatches)
{
    QString text;
    for (int i = 0; i < 50000; ++i) {
        text += QStringLiteral("word ");
    }
    EXPECT_EQ(findAll(text, s("word"), options()).size(), 50000);
}

// ---- replaceAll -----------------------------------------------------------------------------

TEST(TextSearchReplaceAll, ReplacesEveryMatch)
{
    const auto result = replaceAll(s("one two one"), s("one"), s("1"), options());
    EXPECT_EQ(result.text, s("1 two 1"));
    EXPECT_EQ(result.count, 2);
}

TEST(TextSearchReplaceAll, NoMatchLeavesTheTextAlone)
{
    const auto result = replaceAll(s("abc"), s("x"), s("y"), options());
    EXPECT_EQ(result.text, s("abc"));
    EXPECT_EQ(result.count, 0);
}

TEST(TextSearchReplaceAll, AnEmptyReplacementDeletesTheMatches)
{
    const auto result = replaceAll(s("a-b-c"), s("-"), QString(), options());
    EXPECT_EQ(result.text, s("abc"));
    EXPECT_EQ(result.count, 2);
}

TEST(TextSearchReplaceAll, AnEmptyNeedleReplacesNothing)
{
    const auto result = replaceAll(s("abc"), QString(), s("x"), options());
    EXPECT_EQ(result.text, s("abc"));
    EXPECT_EQ(result.count, 0);
}

TEST(TextSearchReplaceAll, AReplacementContainingTheNeedleDoesNotLoop)
{
    const auto result = replaceAll(s("a a"), s("a"), s("aa"), options());
    EXPECT_EQ(result.text, s("aa aa"));
    EXPECT_EQ(result.count, 2);

    const auto nested = replaceAll(s("cat"), s("cat"), s("concatenate cat"), options());
    EXPECT_EQ(nested.text, s("concatenate cat"));
    EXPECT_EQ(nested.count, 1);
}

TEST(TextSearchReplaceAll, ReplacementsOfADifferentLengthKeepThePieceBetweenThem)
{
    EXPECT_EQ(replaceAll(s("x1y1z"), s("1"), s("---"), options()).text, s("x---y---z"));
    EXPECT_EQ(replaceAll(s("x---y---z"), s("---"), s("1"), options()).text, s("x1y1z"));
}

TEST(TextSearchReplaceAll, MatchCaseDecidesWhatIsReplaced)
{
    EXPECT_EQ(replaceAll(s("Cat cat CAT"), s("cat"), s("dog"), options(false)).text,
              s("dog dog dog"));
    EXPECT_EQ(replaceAll(s("Cat cat CAT"), s("cat"), s("dog"), options(true)).text,
              s("Cat dog CAT"));
}

TEST(TextSearchReplaceAll, TheReplacementIsTakenLiterally)
{
    EXPECT_EQ(replaceAll(s("a"), s("a"), s("$1\\n&"), options()).text, s("$1\\n&"));
}

TEST(TextSearchReplaceAll, MatchesAtTheStartAndTheEnd)
{
    EXPECT_EQ(replaceAll(s("ab ab"), s("ab"), s("X"), options()).text, s("X X"));
    EXPECT_EQ(replaceAll(s("ab"), s("ab"), s("X"), options()).text, s("X"));
}

TEST(TextSearchReplaceAll, KeepsLineBreaksAndEmoji)
{
    const auto result = replaceAll(s("a\nb\xF0\x9F\x98\x80"
                                     "a"),
                                   s("a"), s("A"), options());
    EXPECT_EQ(result.text, s("A\nb\xF0\x9F\x98\x80"
                             "A"));
}

// ---- Cross-check against a naive implementation ----------------------------------------------

namespace
{

// A tiny deterministic generator so the test needs no random-number library and never flakes.
class Lcg
{
public:
    explicit Lcg(unsigned seed)
        : m_state(seed)
    {}
    unsigned next(unsigned limit)
    {
        m_state = m_state * 1664525u + 1013904223u;
        return (m_state >> 8u) % limit;
    }

private:
    unsigned m_state;
};

QString randomText(Lcg& random, int length)
{
    static const QChar alphabet[] = {u'a', u'A', u'b', u'B', u' ', u'\n'};
    QString text;
    for (int i = 0; i < length; ++i) {
        text.append(alphabet[random.next(6)]);
    }
    return text;
}

// Compares a window of text one character at a time, the slow obvious way.
bool matchesAt(const QString& text, qsizetype position, const QString& needle, bool matchCase)
{
    if (position + needle.size() > text.size()) {
        return false;
    }
    for (qsizetype i = 0; i < needle.size(); ++i) {
        const QChar a = text.at(position + i);
        const QChar b = needle.at(i);
        if (matchCase ? a != b : a.toLower() != b.toLower()) {
            return false;
        }
    }
    return true;
}

QList<TextRange> naiveFindAll(const QString& text, const QString& needle, bool matchCase)
{
    QList<TextRange> matches;
    for (qsizetype i = 0; i + needle.size() <= text.size();) {
        if (matchesAt(text, i, needle, matchCase)) {
            matches.append({i, needle.size()});
            i += needle.size();
        } else {
            ++i;
        }
    }
    return matches;
}

} // namespace

TEST(TextSearchCrossCheck, FindAllMatchesANaiveImplementation)
{
    Lcg random(12345);
    for (int round = 0; round < 400; ++round) {
        const QString text = randomText(random, static_cast<int>(random.next(40)));
        const QString needle = randomText(random, 1 + static_cast<int>(random.next(3)));
        for (const bool matchCase : {false, true}) {
            EXPECT_EQ(findAll(text, needle, options(matchCase)),
                      naiveFindAll(text, needle, matchCase))
                << "text=" << qPrintable(text.toHtmlEscaped()) << " needle=" << qPrintable(needle);
        }
    }
}

TEST(TextSearchCrossCheck, NextAndPreviousAgreeWithTheListOfMatches)
{
    Lcg random(777);
    for (int round = 0; round < 300; ++round) {
        const QString text = randomText(random, static_cast<int>(random.next(40)));
        const QString needle = randomText(random, 1 + static_cast<int>(random.next(2)));
        const QList<TextRange> all = naiveFindAll(text, needle, false);

        for (qsizetype position = 0; position <= text.size(); ++position) {
            // findNext without wrap: the first match of any kind starting at or after `position`.
            // (Matches here may overlap the list's choices, so test against direct scanning.)
            std::optional<TextRange> expectedNext;
            for (qsizetype i = position; i + needle.size() <= text.size(); ++i) {
                if (matchesAt(text, i, needle, false)) {
                    expectedNext = TextRange{i, needle.size()};
                    break;
                }
            }
            EXPECT_EQ(findNext(text, needle, position, options(false, false)), expectedNext);

            std::optional<TextRange> expectedPrevious;
            for (qsizetype i = position - needle.size(); i >= 0; --i) {
                if (matchesAt(text, i, needle, false)) {
                    expectedPrevious = TextRange{i, needle.size()};
                    break;
                }
            }
            EXPECT_EQ(findPrevious(text, needle, position, options(false, false)),
                      expectedPrevious);

            // With wrapping, "nothing found" only happens when the text has no match at all.
            EXPECT_EQ(findNext(text, needle, position, options(false, true)).has_value(),
                      !all.isEmpty());
            EXPECT_EQ(findPrevious(text, needle, position, options(false, true)).has_value(),
                      !all.isEmpty());
        }
    }
}

TEST(TextSearchCrossCheck, ReplaceAllEqualsStringReplace)
{
    Lcg random(2024);
    for (int round = 0; round < 300; ++round) {
        const QString text = randomText(random, static_cast<int>(random.next(40)));
        const QString needle = randomText(random, 1 + static_cast<int>(random.next(2)));
        const QString replacement = randomText(random, static_cast<int>(random.next(4)));
        for (const bool matchCase : {false, true}) {
            QString expected = text;
            // QString::replace is non-overlapping and left to right, like replaceAll().
            expected.replace(needle, replacement,
                             matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive);
            EXPECT_EQ(replaceAll(text, needle, replacement, options(matchCase)).text, expected);
        }
    }
}
