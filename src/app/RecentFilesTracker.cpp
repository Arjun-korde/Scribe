#include "RecentFilesTracker.h"

#include "EditorTabs.h"
#include "SettingsManager.h"

namespace scribe::app
{

RecentFilesTracker::RecentFilesTracker(editor::EditorTabs& tabs, core::SettingsManager* settings,
                                       QObject* parent)
    : QObject(parent)
    , m_settings(settings)
{
    // Load before listening, so that loading is not mistaken for a change that needs saving.
    if (m_settings != nullptr) {
        m_recent = core::RecentFiles::load(*m_settings);
    }

    connect(&tabs, &editor::EditorTabs::fileUsed, this, [this](const QString& path) {
        if (m_recent.add(path)) {
            save();
            emit changed();
        }
    });
}

void RecentFilesTracker::clear()
{
    if (m_recent.clear()) {
        save();
        emit changed();
    }
}

void RecentFilesTracker::remove(const QString& path)
{
    if (m_recent.remove(path)) {
        save();
        emit changed();
    }
}

void RecentFilesTracker::save()
{
    if (m_settings != nullptr) {
        m_recent.save(*m_settings);
    }
}

} // namespace scribe::app
