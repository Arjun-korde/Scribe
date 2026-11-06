#pragma once

#include <QFont>

#include <algorithm>

namespace scribe::editor
{

/// How every editor looks, shared by all tabs: wrapping, the font, and the zoom on top of it.
/// Keeping these in one value makes them easy to apply to new tabs and to save later.
struct ViewSettings
{
    static constexpr int kMinZoomPercent = 10;
    static constexpr int kMaxZoomPercent = 500;
    static constexpr int kDefaultZoomPercent = 100;
    static constexpr int kZoomStepPercent = 10; ///< One menu click or one wheel notch.

    bool wordWrap = true; ///< Like Windows 11 Notepad.
    QFont font = defaultFont();
    int zoomPercent = kDefaultZoomPercent;

    /// Cascadia Mono, else Consolas, else the system's fixed-width font, at 11 pt.
    static QFont defaultFont();

    static int clampZoom(int percent)
    {
        return std::clamp(percent, kMinZoomPercent, kMaxZoomPercent);
    }

    /// `font` scaled by `zoomPercent`: the font that is actually put on the editors.
    QFont zoomedFont() const;

    friend bool operator==(const ViewSettings&, const ViewSettings&) = default;
};

} // namespace scribe::editor
