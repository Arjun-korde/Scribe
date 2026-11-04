#include "Encoding.h"

#include <QStringConverter>

namespace scribe::core
{

namespace
{

constexpr QByteArrayView kUtf8Bom = "\xEF\xBB\xBF";
constexpr QByteArrayView kUtf16LeBom = "\xFF\xFE";
constexpr QByteArrayView kUtf16BeBom = "\xFE\xFF";

QStringConverter::Encoding converterFor(Encoding encoding)
{
    switch (encoding) {
        case Encoding::Utf8:
            return QStringConverter::Utf8;
        case Encoding::Utf16Le:
            return QStringConverter::Utf16LE;
        case Encoding::Utf16Be:
            return QStringConverter::Utf16BE;
        case Encoding::Ansi:
            return QStringConverter::System;
    }
    return QStringConverter::Utf8;
}

QByteArrayView bomFor(Encoding encoding)
{
    switch (encoding) {
        case Encoding::Utf8:
            return kUtf8Bom;
        case Encoding::Utf16Le:
            return kUtf16LeBom;
        case Encoding::Utf16Be:
            return kUtf16BeBom;
        case Encoding::Ansi:
            return {};
    }
    return {};
}

/// Strict well-formedness check (RFC 3629: no overlongs, no surrogates, nothing above U+10FFFF).
/// QStringDecoder is not used here because it replaces bad bytes without reporting an error.
bool isValidUtf8(const QByteArray& bytes)
{
    const auto at = [&bytes](qsizetype i) { return static_cast<unsigned char>(bytes.at(i)); };
    const auto isContinuation = [](unsigned char byte) {
        return (static_cast<unsigned>(byte) & 0xC0U) == 0x80U;
    };

    const qsizetype size = bytes.size();
    qsizetype i = 0;
    while (i < size) {
        const unsigned char lead = at(i);
        if (lead < 0x80) {
            ++i;
            continue;
        }

        qsizetype length = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;
        if (lead >= 0xC2 && lead <= 0xDF) {
            length = 2;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            length = 3;
            if (lead == 0xE0) {
                secondMin = 0xA0; // rejects overlong forms
            } else if (lead == 0xED) {
                secondMax = 0x9F; // rejects surrogates
            }
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            length = 4;
            if (lead == 0xF0) {
                secondMin = 0x90; // rejects overlong forms
            } else if (lead == 0xF4) {
                secondMax = 0x8F; // rejects code points above U+10FFFF
            }
        } else {
            return false; // stray continuation byte, C0, C1 or F5..FF
        }

        if (i + length > size || at(i + 1) < secondMin || at(i + 1) > secondMax) {
            return false;
        }
        for (qsizetype k = 2; k < length; ++k) {
            if (!isContinuation(at(i + k))) {
                return false;
            }
        }
        i += length;
    }
    return true;
}

/// UTF-16 text that is mostly Latin has a zero byte in the high-byte position of most code
/// units. Ordinary UTF-8 and ANSI text almost never contains zero bytes, so a large share of
/// zeros in one byte position (and almost none in the other) identifies UTF-16 and its order.
std::optional<Encoding> guessUtf16WithoutBom(const QByteArray& bytes)
{
    const qsizetype units = bytes.size() / 2;
    if (units == 0) {
        return std::nullopt;
    }

    qsizetype zerosAtEven = 0; // first byte of a unit is zero: big-endian ASCII
    qsizetype zerosAtOdd = 0;  // second byte of a unit is zero: little-endian ASCII
    for (qsizetype i = 0; i < units; ++i) {
        zerosAtEven += bytes.at(2 * i) == '\0' ? 1 : 0;
        zerosAtOdd += bytes.at(2 * i + 1) == '\0' ? 1 : 0;
    }

    const auto dominates = [units](qsizetype many, qsizetype few) {
        return many * 10 >= units * 3 && few * 20 <= many;
    };

    if (dominates(zerosAtOdd, zerosAtEven)) {
        return Encoding::Utf16Le;
    }
    if (dominates(zerosAtEven, zerosAtOdd)) {
        return Encoding::Utf16Be;
    }
    return std::nullopt;
}

} // namespace

QString encodingName(const EncodingInfo& info)
{
    switch (info.encoding) {
        case Encoding::Utf8:
            return info.hasBom ? QStringLiteral("UTF-8 with BOM") : QStringLiteral("UTF-8");
        case Encoding::Utf16Le:
            return info.hasBom ? QStringLiteral("UTF-16 LE") : QStringLiteral("UTF-16 LE (no BOM)");
        case Encoding::Utf16Be:
            return info.hasBom ? QStringLiteral("UTF-16 BE") : QStringLiteral("UTF-16 BE (no BOM)");
        case Encoding::Ansi:
            return QStringLiteral("ANSI");
    }
    return QStringLiteral("UTF-8");
}

EncodingInfo detectEncoding(const QByteArray& bytes)
{
    if (bytes.startsWith(kUtf8Bom)) {
        return {Encoding::Utf8, true};
    }
    if (bytes.startsWith(kUtf16LeBom)) {
        return {Encoding::Utf16Le, true};
    }
    if (bytes.startsWith(kUtf16BeBom)) {
        return {Encoding::Utf16Be, true};
    }

    // Before the UTF-8 check: zero bytes are valid UTF-8, so UTF-16 text would pass it.
    if (const std::optional<Encoding> utf16 = guessUtf16WithoutBom(bytes)) {
        return {*utf16, false};
    }

    if (isValidUtf8(bytes)) {
        return {Encoding::Utf8, false};
    }
    return {Encoding::Ansi, false};
}

QString decode(const QByteArray& bytes, const EncodingInfo& info)
{
    const QByteArrayView bom = bomFor(info.encoding);
    const qsizetype skip = info.hasBom && bytes.startsWith(bom) ? bom.size() : 0;

    QStringDecoder decoder(converterFor(info.encoding));
    return decoder(QByteArrayView(bytes).sliced(skip));
}

std::optional<QByteArray> encode(const QString& text, const EncodingInfo& info)
{
    QStringEncoder encoder(converterFor(info.encoding));
    QByteArray body = encoder(text);
    if (encoder.hasError()) {
        return std::nullopt;
    }

    // Qt substitutes '?' for unrepresentable characters (ANSI) without reporting an error, so
    // prove the bytes decode back to exactly the text we were given.
    QStringDecoder verifier(converterFor(info.encoding));
    if (QString(verifier(body)) != text) {
        return std::nullopt;
    }

    if (!info.hasBom) {
        return body;
    }
    return QByteArray(bomFor(info.encoding)) + body;
}

} // namespace scribe::core
