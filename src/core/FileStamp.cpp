#include "FileStamp.h"

#include <QDateTime>
#include <QFileInfo>
#include <QTimeZone>

namespace scribe::core
{

FileStamp FileStamp::of(const QString& path)
{
    if (path.isEmpty()) {
        return {};
    }
    const QFileInfo info(path); // a new object, so nothing is cached
    if (!info.exists() || !info.isFile()) {
        return {};
    }
    return {true, info.size(), info.lastModified(QTimeZone::UTC).toMSecsSinceEpoch()};
}

} // namespace scribe::core
