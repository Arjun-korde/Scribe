#include "FindBarPersistence.h"

#include "FindBar.h"
#include "SettingsManager.h"

namespace scribe::editor
{

FindBarPersistence::FindBarPersistence(FindBar& bar, core::SettingsManager& settings,
                                       QObject* parent)
    : QObject(parent)
{
    // Apply the saved options BEFORE listening, so that loading is not mistaken for a change that
    // needs saving.
    bar.setMatchCase(settings.boolValue(kMatchCaseKey, bar.matchCase()));
    bar.setWrapAround(settings.boolValue(kWrapAroundKey, bar.wrapAround()));

    connect(&bar, &FindBar::optionsChanged, this, [&bar, &settings] {
        settings.setBool(kMatchCaseKey, bar.matchCase());
        settings.setBool(kWrapAroundKey, bar.wrapAround());
    });
}

} // namespace scribe::editor
