#pragma once

#include <QString>

namespace scribe::core
{

/// Storage for the text of one document. Business logic reads and edits text through this
/// interface instead of touching the editor widget, so a different implementation (for example
/// a memory-mapped backend for very large files) can be added later without rewrites.
///
/// Text is always stored with '\n' as the only line break. Converting to and from the file's
/// real line ending is the job of Document. Positions and lengths count UTF-16 code units, like
/// QString.
class IDocumentBackend
{
public:
    IDocumentBackend() = default;
    virtual ~IDocumentBackend() = default;

    IDocumentBackend(const IDocumentBackend&) = delete;
    IDocumentBackend& operator=(const IDocumentBackend&) = delete;

    /// Number of code units in the document.
    virtual qsizetype length() const = 0;

    /// Number of lines. An empty document has one (empty) line; every '\n' adds another.
    virtual qsizetype lineCount() const = 0;

    /// The whole text.
    virtual QString text() const = 0;

    /// `count` code units starting at `pos`. The range is clamped to the document.
    virtual QString text(qsizetype pos, qsizetype count) const = 0;

    /// The line at `index` without its '\n'. An out-of-range index gives an empty string.
    virtual QString line(qsizetype index) const = 0;

    /// Replaces the whole document.
    virtual void setText(const QString& text) = 0;

    /// Inserts `text` before `pos`. Returns false, changing nothing, unless 0 <= pos <= length().
    virtual bool insert(qsizetype pos, const QString& text) = 0;

    /// Removes `count` code units starting at `pos`. Returns false, changing nothing, unless
    /// pos >= 0, count >= 0 and the range lies inside the document.
    virtual bool remove(qsizetype pos, qsizetype count) = 0;
};

} // namespace scribe::core
