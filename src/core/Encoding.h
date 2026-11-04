#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <optional>

namespace scribe::core
{

enum class Encoding : std::uint8_t
{
    Utf8,
    Utf16Le,
    Utf16Be,
    Ansi, ///< The Windows system code page (usually Windows-1252).
};

/// An encoding plus whether the file starts with a byte order mark. The BOM is kept separate so
/// that a file opened without one is saved without one.
struct EncodingInfo
{
    Encoding encoding = Encoding::Utf8;
    bool hasBom = false;

    friend bool operator==(const EncodingInfo&, const EncodingInfo&) = default;
};

/// Display name for the status bar: "UTF-8", "UTF-8 with BOM", "UTF-16 LE", "UTF-16 LE (no BOM)",
/// "UTF-16 BE", "UTF-16 BE (no BOM)" or "ANSI".
QString encodingName(const EncodingInfo& info);

/// Guesses the encoding of raw file bytes. In order: BOM (certain), UTF-16 without BOM (many
/// zero bytes in one byte position), valid UTF-8, then ANSI as the last resort. Empty input is
/// UTF-8 without BOM.
EncodingInfo detectEncoding(const QByteArray& bytes);

/// Decodes `bytes` using `info`, skipping the BOM when `info.hasBom` is set. Invalid sequences
/// become U+FFFD, so callers that must not lose data should only use an encoding returned by
/// detectEncoding() or chosen explicitly by the user.
QString decode(const QByteArray& bytes, const EncodingInfo& info);

/// Encodes `text` using `info`, writing the BOM when `info.hasBom` is set. Returns std::nullopt
/// when some character cannot be represented (for example an emoji in ANSI), so the caller can
/// refuse to save instead of silently writing `?`.
std::optional<QByteArray> encode(const QString& text, const EncodingInfo& info);

} // namespace scribe::core
