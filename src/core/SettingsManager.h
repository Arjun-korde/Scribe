#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

namespace scribe::core
{

/// The preferences that survive closing the application, kept in a plain INI file:
///
///   [settings]
///   version=1
///   [view]
///   wordWrap=true
///   zoom=120
///
/// Keys are written "section/name" (for example "view/zoom"). Every value has a default that the
/// caller supplies, so a missing file, a missing key, or a value that was damaged or edited by
/// hand into nonsense all give the default and never an error.
///
/// Changes are written to disk at once, because settings change rarely and this way nothing is
/// lost if the program crashes. The file and its folder are created by the first change; just
/// reading never creates anything. If the file cannot be written the program carries on: changes
/// are simply not saved, and isHealthy() says so.
///
/// Not thread-safe: use it from the GUI thread only.
class SettingsManager
{
public:
    /// The version written into new settings files, for migrating older files in the future.
    static constexpr int kCurrentVersion = 1;

    /// Lists longer than this are cut short when read, so a damaged file cannot make the program
    /// loop for a very long time.
    static constexpr int kMaxListItems = 1000;

    /// Uses the file at `filePath` (it need not exist yet). Tests pass a temporary file, so they
    /// never touch the real settings.
    explicit SettingsManager(const QString& filePath);

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    /// "%APPDATA%\Scribe\scribe.ini" (written with forward slashes).
    [[nodiscard]] static QString defaultFilePath();

    [[nodiscard]] const QString& filePath() const { return m_filePath; }

    // ---- Reading: a missing or invalid value gives `defaultValue`. -----------------------------

    /// "true" / "false" / "1" / "0", in any letter case. Anything else is invalid.
    [[nodiscard]] bool boolValue(const QString& key, bool defaultValue) const;

    /// A whole number from `minimum` to `maximum` (both allowed). Text that is not a number, and
    /// numbers outside the range, are invalid: a value like zoom=99999 is a sign of damage, not
    /// something to guess a meaning for.
    [[nodiscard]] int intValue(const QString& key, int defaultValue, int minimum,
                               int maximum) const;

    [[nodiscard]] QString stringValue(const QString& key, const QString& defaultValue) const;

    /// A list saved with setStringList(), in its original order; empty if there is none.
    /// Reads at most `maxItems` entries.
    [[nodiscard]] QStringList stringList(const QString& key, int maxItems = kMaxListItems) const;

    [[nodiscard]] bool contains(const QString& key) const;

    /// The settings file's version: kCurrentVersion for a file this program wrote, a higher number
    /// for a file from a newer program, and 0 if nothing has been written yet.
    [[nodiscard]] int version() const;

    // ---- Writing: saved to disk immediately; nothing is written if the value is unchanged. -----

    void setBool(const QString& key, bool value);
    void setInt(const QString& key, int value);
    void setString(const QString& key, const QString& value);

    /// Each entry is stored on its own, so commas, quotes and other special characters in the
    /// strings (all legal in file names) are safe. An empty list removes the key.
    void setStringList(const QString& key, const QStringList& values);

    void remove(const QString& key);

    /// False once the file turned out to be unreadable, malformed, or impossible to write. The
    /// program keeps working with defaults / unsaved changes; this is for logging and for tests.
    [[nodiscard]] bool isHealthy() const;

private:
    [[nodiscard]] QString textOf(const QString& key) const;
    void setText(const QString& key, const QString& text);
    void writeVersionIfMissing();

    /// Writes pending changes to disk, creating the folder first.
    void commit();

    QString m_filePath;
    // Mutable because Qt's array reading (beginReadArray) is not a const operation.
    mutable QSettings m_settings;
    bool m_couldNotCreateFolder = false;
    bool m_warned = false;
};

} // namespace scribe::core
