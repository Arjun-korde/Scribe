#include "ViewSettingsPersistence.h"

#include "EditorTabs.h"
#include "SettingsManager.h"
#include "ViewSettingsStore.h"

namespace scribe::editor
{

ViewSettingsPersistence::ViewSettingsPersistence(EditorTabs& tabs, core::SettingsManager& settings,
                                                 std::chrono::milliseconds saveDelay,
                                                 QObject* parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(saveDelay);
    connect(&m_timer, &QTimer::timeout, this, &ViewSettingsPersistence::flush);

    // Apply what was saved BEFORE listening for changes, so that loading is not mistaken for a
    // change that needs saving (which would rewrite the file with every start).
    tabs.setViewSettings(ViewSettingsStore::load(settings));

    connect(&tabs, &EditorTabs::viewSettingsChanged, this, [this, &tabs] {
        m_pending = tabs.viewSettings();
        m_dirty = true;
        m_timer.start(); // restarts the wait: only a quiet period triggers the write
    });
}

ViewSettingsPersistence::~ViewSettingsPersistence()
{
    flush();
}

void ViewSettingsPersistence::flush()
{
    if (!m_dirty) {
        return;
    }
    m_timer.stop();
    m_dirty = false;
    ViewSettingsStore::save(m_settings, m_pending);
}

} // namespace scribe::editor
