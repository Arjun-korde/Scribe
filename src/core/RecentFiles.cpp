#include "RecentFiles.h"

#include "SettingsManager.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace scribe::core
{

namespace
{

constexpr auto kListKey = "recent/files";

#ifdef Q_OS_WIN
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseInsensitive;
#else
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseSensitive;
#endif

} // namespace

RecentFiles::RecentFiles(int maxItems)
    : m_maxItems(std::max(maxItems, 1))
{}

QString RecentFiles::normalized(const QString& path)
{
    if (path.trimmed().isEmpty()) {
        return {};
    }
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

qsizetype RecentFiles::indexOf(const QString& normalizedPath) const
{
    for (qsizetype i = 0; i < m_paths.size(); ++i) {
        if (m_paths.at(i).compare(normalizedPath, kPathCase) == 0) {
            return i;
        }
    }
    return -1;
}

bool RecentFiles::contains(const QString& path) const
{
    return indexOf(normalized(path)) >= 0;
}

bool RecentFiles::add(const QString& path)
{
    const QString entry = normalized(path);
    if (entry.isEmpty()) {
        return false;
    }
    const qsizetype existing = indexOf(entry);
    if (existing == 0 && m_paths.constFirst() == entry) {
        return false; // already on top, spelled the same way
    }
    if (existing >= 0) {
        m_paths.removeAt(existing);
    }
    m_paths.prepend(entry); // the newest spelling wins, for example a corrected letter case
    while (m_paths.size() > m_maxItems) {
        m_paths.removeLast();
    }
    return true;
}

bool RecentFiles::remove(const QString& path)
{
    const qsizetype existing = indexOf(normalized(path));
    if (existing < 0) {
        return false;
    }
    m_paths.removeAt(existing);
    return true;
}

bool RecentFiles::clear()
{
    const bool hadEntries = !m_paths.isEmpty();
    m_paths.clear();
    return hadEntries;
}

RecentFiles RecentFiles::load(const SettingsManager& settings, int maxItems)
{
    RecentFiles result(maxItems);
    const QStringList saved = settings.stringList(QLatin1String(kListKey));
    // Saved newest first; adding oldest first rebuilds the same order and applies the same rules
    // (no repeats, size limit, the oldest dropped first) to whatever the file contained.
    for (auto it = saved.crbegin(); it != saved.crend(); ++it) {
        if (QFileInfo(*it).isAbsolute()) {
            result.add(*it);
        }
    }
    return result;
}

void RecentFiles::save(SettingsManager& settings) const
{
    settings.setStringList(QLatin1String(kListKey), m_paths);
}

} // namespace scribe::core
