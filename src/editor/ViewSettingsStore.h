#pragma once

#include "ViewSettings.h"

#include <QLatin1String>

namespace scribe::core
{
class SettingsManager;
}

namespace scribe::editor
{

/// Saves and loads ViewSettings (word wrap, font, zoom) in the settings file:
///
///   [view]
///   wordWrap=true
///   fontFamily=Cascadia Mono
///   fontSize=11
///   fontWeight=400
///   fontItalic=false
///   zoom=100
///
/// Loading never fails: anything missing or invalid gives the default for that part, so one bad
/// value does not cost the others. The font keeps its family, size, weight and italic (what the
/// font dialog offers as "Regular / Bold / Italic / Bold Italic"). Underline and strikeout are
/// not remembered.
class ViewSettingsStore
{
public:
    static constexpr QLatin1String kWordWrapKey{"view/wordWrap"};
    static constexpr QLatin1String kFontFamilyKey{"view/fontFamily"};
    static constexpr QLatin1String kFontSizeKey{"view/fontSize"};
    static constexpr QLatin1String kFontWeightKey{"view/fontWeight"};
    static constexpr QLatin1String kFontItalicKey{"view/fontItalic"};
    static constexpr QLatin1String kZoomKey{"view/zoom"};

    /// The saved settings, with defaults for whatever is missing or invalid:
    /// - a zoom outside 10..500 or not a number;
    /// - a font size outside 1..200 points or not a number;
    /// - a weight outside 1..1000;
    /// - a font family that is not installed (a font that was removed since it was saved).
    [[nodiscard]] static ViewSettings load(const core::SettingsManager& settings);

    /// Writes all of `view`. A font without a point size (sized in pixels) has its size left out,
    /// so the default size is used next time.
    static void save(core::SettingsManager& settings, const ViewSettings& view);
};

} // namespace scribe::editor
