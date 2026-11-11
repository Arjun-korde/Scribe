#pragma once

#include "Encoding.h"
#include "FileStamp.h"
#include "IDocumentBackend.h"
#include "LineEnding.h"

#include <QString>

#include <cstdint>
#include <memory>

namespace scribe::core
{

enum class DocumentError : std::uint8_t
{
    None,
    CannotRead,      ///< The file does not exist or cannot be opened for reading.
    InvalidEncoding, ///< The bytes cannot be decoded and re-encoded without changing them.
    CannotEncode,    ///< The text has characters the chosen encoding cannot represent.
    CannotWrite,     ///< The file (or its folder) cannot be written.
};

/// Outcome of load() and save(). `detail` is technical text from the OS, for logs.
struct IoResult
{
    DocumentError error = DocumentError::None;
    QString detail;

    bool ok() const { return error == DocumentError::None; }
};

/// One open text document: its text (in an IDocumentBackend), the file it came from, and how that
/// file must be written back (encoding, BOM, line ending).
///
/// The backend always holds '\n' line breaks. load() converts them in, save() converts them out
/// using lineEnding(). A file with mixed line endings is therefore saved with its most common one.
class Document
{
public:
    /// An empty, unsaved document: UTF-8 without BOM, CRLF line endings, in-memory storage.
    Document();
    explicit Document(std::unique_ptr<IDocumentBackend> backend);

    const IDocumentBackend& backend() const { return *m_backend; }

    /// The path of the file on disk, empty for a document that was never saved.
    const QString& filePath() const { return m_filePath; }

    bool isModified() const { return m_modified; }

    /// How the file looked on disk when it was last loaded or saved by this document. Compare it
    /// with FileStamp::of(filePath()) to find out whether another program changed the file since.
    const FileStamp& diskStamp() const { return m_diskStamp; }

    /// Takes the file's current state as the known one, so a change that was already noticed (and
    /// deliberately ignored) is not reported again. Does not read the file.
    void refreshDiskStamp();

    const EncodingInfo& encoding() const { return m_encoding; }
    LineEnding lineEnding() const { return m_lineEnding; }

    /// Changing how the file is written counts as a modification.
    void setEncoding(const EncodingInfo& encoding);
    void setLineEnding(LineEnding lineEnding);

    /// Edits. Each one marks the document modified. `text` may contain any line breaks; they are
    /// normalized to '\n'. insert() and remove() return false, changing nothing, for a bad range.
    void setText(const QString& text);
    bool insert(qsizetype pos, const QString& text);
    bool remove(qsizetype pos, qsizetype count);

    /// Replaces this document with the file at `path`. On failure the document is unchanged.
    IoResult load(const QString& path);

    /// Writes to the current file path. A failed save leaves the file on disk untouched and the
    /// document modified.
    IoResult save();

    /// Writes to `path` and, on success, makes it the document's path.
    IoResult saveAs(const QString& path);

private:
    std::unique_ptr<IDocumentBackend> m_backend;
    QString m_filePath;
    EncodingInfo m_encoding;
    LineEnding m_lineEnding = LineEnding::Crlf;
    FileStamp m_diskStamp;
    bool m_modified = false;
};

} // namespace scribe::core
