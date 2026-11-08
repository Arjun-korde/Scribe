#pragma once

#include <QList>
#include <QRect>

#include <optional>

namespace scribe::core
{

class SettingsManager;

/// Where the main window was when it was last closed: its size and position as a normal window
/// (even when it was maximized, so un-maximizing later goes back to the right place), and whether
/// it was maximized.
struct WindowPlacement
{
    QRect rect; ///< The window's client area, in screen coordinates.
    bool maximized = false;

    friend bool operator==(const WindowPlacement&, const WindowPlacement&) = default;
};

/// The saved placement, or std::nullopt if there is none or it cannot be trusted. The file looks
/// like this:
///
///   [window]
///   x=120
///   y=80
///   width=900
///   height=600
///   maximized=false
///
/// All four numbers must be present and sensible (a size of at least 320 x 200, nothing absurdly
/// large); if any is missing or damaged the whole position is ignored rather than guessed, and
/// the window gets its default size.
[[nodiscard]] std::optional<WindowPlacement> loadWindowPlacement(const SettingsManager& settings);

void saveWindowPlacement(SettingsManager& settings, const WindowPlacement& placement);

/// Decides where to put a window that was saved at `wanted`, given the areas of the screens that
/// are connected now (their usable parts, without the taskbar). `titleBarHeight` is the thickness
/// of the window's title bar, which sits above the client area `wanted`.
///
/// - If the window is still where it was (fully on a screen), the answer is `wanted` unchanged.
/// - If it is partly off a screen, or bigger than the screen, it is shrunk and moved fully into
///   view (title bar included) on the screen that held most of it.
/// - If it is on none of the connected screens (for example it was on a monitor that has been
///   unplugged), the answer is std::nullopt: the caller should use the default size and let the
///   system place it, since a window nobody can reach is worse than one in the wrong place.
[[nodiscard]] std::optional<QRect> fitToScreens(const QRect& wanted, const QList<QRect>& screens,
                                                int titleBarHeight);

} // namespace scribe::core
