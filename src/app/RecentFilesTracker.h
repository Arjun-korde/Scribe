#pragma once

#include "RecentFiles.h"

#include <QObject>

namespace scribe::core
{
class SettingsManager;
}

namespace scribe::editor
{
class EditorTabs;
}

namespace scribe::app
{

/// Keeps the list of recently used files up to date and remembered between runs.
///
/// It listens to EditorTabs::fileUsed, so every successful open (File > Open, drag and drop,
/// switching to a file that is already open) and every Save As puts that file at the top. The list
/// is loaded from the settings file when this is created and saved after each change; with no
/// settings store the list simply lives as long as the program runs.
///
/// The tabs and the settings store must outlive this object.
class RecentFilesTracker : public QObject
{
    Q_OBJECT

public:
    RecentFilesTracker(editor::EditorTabs& tabs, core::SettingsManager* settings,
                       QObject* parent = nullptr);

    [[nodiscard]] const QStringList& paths() const { return m_recent.paths(); }
    [[nodiscard]] bool isEmpty() const { return m_recent.isEmpty(); }

    /// Forgets every entry.
    void clear();

    /// Forgets one entry, for example a file that no longer exists.
    void remove(const QString& path);

signals:
    /// The list changed.
    void changed();

private:
    void save();

    core::RecentFiles m_recent;
    core::SettingsManager* m_settings;
};

} // namespace scribe::app
