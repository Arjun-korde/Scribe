#include "InMemoryBackend.h"

#include <gtest/gtest.h>

using scribe::core::IDocumentBackend;
using scribe::core::InMemoryBackend;

TEST(InMemoryBackend, StartsEmptyWithOneLine)
{
    InMemoryBackend backend;
    EXPECT_EQ(backend.length(), 0);
    EXPECT_EQ(backend.lineCount(), 1);
    EXPECT_EQ(backend.text(), QString());
    EXPECT_EQ(backend.line(0), QString());
}

TEST(InMemoryBackend, SetTextReplacesEverything)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("one\ntwo"));
    backend.setText(QStringLiteral("x"));
    EXPECT_EQ(backend.text(), QStringLiteral("x"));
    EXPECT_EQ(backend.length(), 1);
}

TEST(InMemoryBackend, CountsLines)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("a\nb\nc"));
    EXPECT_EQ(backend.lineCount(), 3);

    backend.setText(QStringLiteral("a\n"));
    EXPECT_EQ(backend.lineCount(), 2); // a trailing newline starts an empty last line

    backend.setText(QStringLiteral("\n\n"));
    EXPECT_EQ(backend.lineCount(), 3);
}

TEST(InMemoryBackend, ReturnsLinesWithoutTheirBreak)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("first\n\nthird\n"));
    EXPECT_EQ(backend.line(0), QStringLiteral("first"));
    EXPECT_EQ(backend.line(1), QString());
    EXPECT_EQ(backend.line(2), QStringLiteral("third"));
    EXPECT_EQ(backend.line(3), QString());
}

TEST(InMemoryBackend, OutOfRangeLineIsEmpty)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("a\nb"));
    EXPECT_EQ(backend.line(-1), QString());
    EXPECT_EQ(backend.line(2), QString());
    EXPECT_EQ(backend.line(100), QString());
}

TEST(InMemoryBackend, ReturnsClampedRanges)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("abcdef"));
    EXPECT_EQ(backend.text(1, 3), QStringLiteral("bcd"));
    EXPECT_EQ(backend.text(4, 100), QStringLiteral("ef"));
    EXPECT_EQ(backend.text(6, 1), QString());
    EXPECT_EQ(backend.text(-1, 2), QString());
    EXPECT_EQ(backend.text(0, 0), QString());
    EXPECT_EQ(backend.text(0, -1), QString());
}

TEST(InMemoryBackend, InsertsAtStartMiddleAndEnd)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("ace"));
    EXPECT_TRUE(backend.insert(1, QStringLiteral("b")));
    EXPECT_TRUE(backend.insert(0, QStringLiteral(">")));
    EXPECT_TRUE(backend.insert(backend.length(), QStringLiteral("<")));
    EXPECT_EQ(backend.text(), QStringLiteral(">abce<"));
}

TEST(InMemoryBackend, RejectsInsertOutsideTheDocument)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("abc"));
    EXPECT_FALSE(backend.insert(-1, QStringLiteral("x")));
    EXPECT_FALSE(backend.insert(4, QStringLiteral("x"))); // QString would pad with spaces here
    EXPECT_EQ(backend.text(), QStringLiteral("abc"));
}

TEST(InMemoryBackend, RemovesRanges)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("abcdef"));
    EXPECT_TRUE(backend.remove(1, 2));
    EXPECT_EQ(backend.text(), QStringLiteral("adef"));
    EXPECT_TRUE(backend.remove(2, 2)); // up to the very end
    EXPECT_EQ(backend.text(), QStringLiteral("ad"));
    EXPECT_TRUE(backend.remove(1, 0));
    EXPECT_EQ(backend.text(), QStringLiteral("ad"));
}

TEST(InMemoryBackend, RejectsRemoveOutsideTheDocument)
{
    InMemoryBackend backend;
    backend.setText(QStringLiteral("abc"));
    EXPECT_FALSE(backend.remove(-1, 1));
    EXPECT_FALSE(backend.remove(1, -1));
    EXPECT_FALSE(backend.remove(2, 2)); // runs past the end
    EXPECT_FALSE(backend.remove(4, 0));
    EXPECT_EQ(backend.text(), QStringLiteral("abc"));
}

TEST(InMemoryBackend, WorksThroughTheInterface)
{
    InMemoryBackend concrete;
    IDocumentBackend& backend = concrete;
    backend.setText(QStringLiteral("hi"));
    EXPECT_TRUE(backend.insert(2, QStringLiteral("!")));
    EXPECT_EQ(backend.text(), QStringLiteral("hi!"));
}
