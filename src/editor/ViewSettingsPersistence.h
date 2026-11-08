#pragma once

#include "ViewSettings.h"

#include <QObject>
#include <QTimer>

#include <chrono>

namespace scribe::core
{
class SettingsManager;
}

namespace scribe::editor
{

class EditorTabs;

/// Keeps the tabs' view settings (word wrap, font, zoom) in the settings file.
///
/// - When created it loads the saved settings and applies them to the tabs.
/// - After that, every change to the tabs' view settings is saved, but only after a short quiet
///   period, so that turning the mouse wheel through ten zoom steps writes the file once.
/// - Anything still waiting is written when flush() is called and when this object is destroyed
///   (which happens when the window closes), so a change made just before quitting is not lost.
///
/// The settings store must outlive this object. The tabs may be destroyed first: the object keeps
/// its own copy of what it still has to save.
class ViewSettingsPersistence : public QObject
{
    Q_OBJECT

public:
    static constexpr std::chrono::milliseconds kDefaultSaveDelay{400};

    ViewSettingsPersistence(EditorTabs& tabs, core::SettingsManager& settings,
                            std::chrono::milliseconds saveDelay = kDefaultSaveDelay,
                            QObject* parent = nullptr);
    ~ViewSettingsPersistence() override;

    /// Writes any waiting change now.
    void flush();

    /// Whether a change is waiting to be written.
    [[nodiscard]] bool hasPendingChanges() const { return m_dirty; }

private:
    core::SettingsManager& m_settings;
    ViewSettings m_pending;
    bool m_dirty = false;
    QTimer m_timer;
};

} // namespace scribe::editor
