#pragma once

#include <QString>
#include <QtTypes>

namespace scribe::core
{

/// What a file looked like on disk at one moment: whether it existed, its size and its last
/// modification time. Two stamps of the same path that differ mean the file was changed (or
/// deleted, or created) in between. Used to notice edits made by other programs.
struct FileStamp
{
    bool exists = false;
    qint64 size = 0;
    qint64 modifiedMs = 0; ///< Milliseconds since the epoch (UTC); 0 if the file does not exist.

    /// Reads the current state of `path`. Never cached: every call asks the file system.
    [[nodiscard]] static FileStamp of(const QString& path);

    friend bool operator==(const FileStamp&, const FileStamp&) = default;
};

} // namespace scribe::core
