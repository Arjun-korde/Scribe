#pragma once

#include <QString>
#include <QStringList>

namespace scribe::core
{

class SettingsManager;

/// The files opened or saved most recently, newest first, without duplicates. This is only the
/// list; showing it in a menu and opening an entry is done elsewhere.
///
/// Paths are stored as absolute paths with forward slashes. Two spellings of the same file
/// ("C:\a\..\b.txt" and "c:/b.txt") count as one; on Windows, where file names ignore letter case,
/// so does a different case. Using a file again moves it to the top, and the oldest entries fall
/// off the end once the list is full.
class RecentFiles
{
public:
    static constexpr int kDefaultMaxItems = 10;

    explicit RecentFiles(int maxItems = kDefaultMaxItems);

    /// Puts `path` at the top. An empty path is ignored. Returns true if the list changed.
    bool add(const QString& path);

    /// Takes `path` out of the list (for example because the file no longer exists). Returns true
    /// if it was there.
    bool remove(const QString& path);

    /// Returns true if the list was not already empty.
    bool clear();

    [[nodiscard]] const QStringList& paths() const { return m_paths; }
    [[nodiscard]] qsizetype size() const { return m_paths.size(); }
    [[nodiscard]] bool isEmpty() const { return m_paths.isEmpty(); }
    [[nodiscard]] int maxItems() const { return m_maxItems; }
    [[nodiscard]] bool contains(const QString& path) const;

    /// The same path in the one form the list stores (absolute, forward slashes, cleaned up).
    [[nodiscard]] static QString normalized(const QString& path);

    // ---- Keeping the list between runs: the "recent/files" list in the settings file. ----------

    /// Reads the saved list. Entries that are empty, not absolute paths, or repeated are skipped,
    /// and anything beyond `maxItems` is dropped, so a damaged or hand-edited file is harmless.
    [[nodiscard]] static RecentFiles load(const SettingsManager& settings,
                                          int maxItems = kDefaultMaxItems);

    void save(SettingsManager& settings) const;

private:
    [[nodiscard]] qsizetype indexOf(const QString& normalizedPath) const;

    QStringList m_paths;
    int m_maxItems;
};

} // namespace scribe::core
