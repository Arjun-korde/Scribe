#pragma once

#include <QLatin1String>
#include <QObject>

namespace scribe::core
{
class SettingsManager;
}

namespace scribe::editor
{

class FindBar;

/// Remembers the find bar's two options between runs:
///
///   [find]
///   matchCase=false
///   wrapAround=true
///
/// When created it applies the saved options to the bar (a missing or invalid value leaves that
/// option as it is); afterwards every change is saved at once, since people rarely toggle them.
/// The text being searched for is not remembered.
///
/// The settings store must outlive this object.
class FindBarPersistence : public QObject
{
    Q_OBJECT

public:
    static constexpr QLatin1String kMatchCaseKey{"find/matchCase"};
    static constexpr QLatin1String kWrapAroundKey{"find/wrapAround"};

    FindBarPersistence(FindBar& bar, core::SettingsManager& settings, QObject* parent = nullptr);
};

} // namespace scribe::editor
