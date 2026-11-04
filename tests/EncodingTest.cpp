#include "Encoding.h"

#include <gtest/gtest.h>

using scribe::core::decode;
using scribe::core::detectEncoding;
using scribe::core::encode;
using scribe::core::Encoding;
using scribe::core::EncodingInfo;

namespace
{

QByteArray hex(const char* digits)
{
    return QByteArray::fromHex(digits);
}

} // namespace

TEST(EncodingDetect, EmptyIsUtf8WithoutBom)
{
    EXPECT_EQ(detectEncoding(QByteArray()), (EncodingInfo{Encoding::Utf8, false}));
}

TEST(EncodingDetect, DetectsEachBom)
{
    EXPECT_EQ(detectEncoding(hex("EFBBBF") + "hi"), (EncodingInfo{Encoding::Utf8, true}));
    EXPECT_EQ(detectEncoding(hex("FFFE6800")), (EncodingInfo{Encoding::Utf16Le, true}));
    EXPECT_EQ(detectEncoding(hex("FEFF0068")), (EncodingInfo{Encoding::Utf16Be, true}));
}

TEST(EncodingDetect, BomOnlyFilesAreDetected)
{
    EXPECT_EQ(detectEncoding(hex("EFBBBF")), (EncodingInfo{Encoding::Utf8, true}));
    EXPECT_EQ(detectEncoding(hex("FFFE")), (EncodingInfo{Encoding::Utf16Le, true}));
}

TEST(EncodingDetect, AsciiAndValidUtf8AreUtf8)
{
    EXPECT_EQ(detectEncoding("plain ascii\r\n"), (EncodingInfo{Encoding::Utf8, false}));
    EXPECT_EQ(detectEncoding(hex("C3A9E282ACF09F9880")), (EncodingInfo{Encoding::Utf8, false}));
}

TEST(EncodingDetect, InvalidUtf8FallsBackToAnsi)
{
    EXPECT_EQ(detectEncoding(hex("636166E9")), (EncodingInfo{Encoding::Ansi, false})); // lone E9
    EXPECT_EQ(detectEncoding(hex("E282")), (EncodingInfo{Encoding::Ansi, false}));     // truncated
    EXPECT_EQ(detectEncoding(hex("C0AF")), (EncodingInfo{Encoding::Ansi, false}));     // overlong
}

TEST(EncodingDetect, Utf16WithoutBomIsGuessedFromZeroBytes)
{
    EXPECT_EQ(detectEncoding(hex("480065006C006C006F00")),
              (EncodingInfo{Encoding::Utf16Le, false}));
    EXPECT_EQ(detectEncoding(hex("00480065006C006C006F")),
              (EncodingInfo{Encoding::Utf16Be, false}));
}

TEST(EncodingDetect, SingleStrayZeroByteDoesNotMeanUtf16)
{
    EXPECT_EQ(detectEncoding(QByteArray("hello world\0 more text", 22)),
              (EncodingInfo{Encoding::Utf8, false}));
}

TEST(EncodingRoundTrip, EveryEncodingKeepsTheExactBytes)
{
    const QString text = QStringLiteral("café € line\r\nsecond");
    const QString ansiText = QStringLiteral("café line\r\nsecond");

    const EncodingInfo unicode[] = {
        {Encoding::Utf8, false},    {Encoding::Utf8, true},    {Encoding::Utf16Le, true},
        {Encoding::Utf16Le, false}, {Encoding::Utf16Be, true}, {Encoding::Utf16Be, false},
    };
    for (const EncodingInfo& info : unicode) {
        const std::optional<QByteArray> bytes = encode(text, info);
        ASSERT_TRUE(bytes.has_value());
        EXPECT_EQ(decode(*bytes, info), text);
        EXPECT_EQ(encode(decode(*bytes, info), info), bytes);
    }

    const EncodingInfo ansi{Encoding::Ansi, false};
    const std::optional<QByteArray> ansiBytes = encode(ansiText, ansi);
    ASSERT_TRUE(ansiBytes.has_value());
    EXPECT_EQ(decode(*ansiBytes, ansi), ansiText);
    EXPECT_EQ(encode(decode(*ansiBytes, ansi), ansi), ansiBytes);
}

TEST(EncodingRoundTrip, DetectThenDecodeThenEncodeIsLossless)
{
    const QByteArray files[] = {
        hex("EFBBBF") + "bom utf8",
        hex("6E6F20626F6D207574663820C3A9"),
        hex("FFFE48006900"),
        hex("480069002100"),
        hex("00480069"),
        hex("636166E9"),
    };
    for (const QByteArray& file : files) {
        const EncodingInfo info = detectEncoding(file);
        EXPECT_EQ(encode(decode(file, info), info), file) << qPrintable(file.toHex());
    }
}

TEST(EncodingDecode, SkipsBomAndKeepsTextClean)
{
    EXPECT_EQ(decode(hex("EFBBBF") + "hi", {Encoding::Utf8, true}), QStringLiteral("hi"));
    EXPECT_EQ(decode(hex("FFFE68006900"), {Encoding::Utf16Le, true}), QStringLiteral("hi"));
}

TEST(EncodingEncode, WritesBomOnlyWhenAsked)
{
    EXPECT_EQ(encode(QStringLiteral("hi"), {Encoding::Utf8, true}), hex("EFBBBF") + "hi");
    EXPECT_EQ(encode(QStringLiteral("hi"), {Encoding::Utf8, false}), QByteArray("hi"));
    EXPECT_EQ(encode(QStringLiteral("hi"), {Encoding::Utf16Be, true}), hex("FEFF00680069"));
}

TEST(EncodingEncode, UnrepresentableTextIsRefusedNotReplaced)
{
    // No ANSI code page contains an emoji.
    EXPECT_EQ(encode(QStringLiteral("smile \U0001F600"), {Encoding::Ansi, false}), std::nullopt);
}

TEST(EncodingName, MatchesStatusBarWording)
{
    using scribe::core::encodingName;
    EXPECT_EQ(encodingName({Encoding::Utf8, false}), QStringLiteral("UTF-8"));
    EXPECT_EQ(encodingName({Encoding::Utf8, true}), QStringLiteral("UTF-8 with BOM"));
    EXPECT_EQ(encodingName({Encoding::Utf16Le, true}), QStringLiteral("UTF-16 LE"));
    EXPECT_EQ(encodingName({Encoding::Utf16Be, false}), QStringLiteral("UTF-16 BE (no BOM)"));
    EXPECT_EQ(encodingName({Encoding::Ansi, false}), QStringLiteral("ANSI"));
}
