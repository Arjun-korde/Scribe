#include "WindowPlacement.h"

#include "SettingsManager.h"

#include <algorithm>
#include <limits>

namespace scribe::core
{

namespace
{

constexpr auto kXKey = "window/x";
constexpr auto kYKey = "window/y";
constexpr auto kWidthKey = "window/width";
constexpr auto kHeightKey = "window/height";
constexpr auto kMaximizedKey = "window/maximized";

constexpr int kMinWidth = 320;
constexpr int kMinHeight = 200;
constexpr int kMaxSize = 30000;        // wider than any real screen setup
constexpr int kMaxCoordinate = 100000; // positions on a very large or odd monitor arrangement

// Marks "missing or invalid" while reading: it is outside every accepted range.
constexpr int kInvalid = std::numeric_limits<int>::min();

qint64 areaOf(const QRect& rect)
{
    return rect.isValid() ? static_cast<qint64>(rect.width()) * rect.height() : 0;
}

} // namespace

std::optional<WindowPlacement> loadWindowPlacement(const SettingsManager& settings)
{
    const int x =
        settings.intValue(QLatin1String(kXKey), kInvalid, -kMaxCoordinate, kMaxCoordinate);
    const int y =
        settings.intValue(QLatin1String(kYKey), kInvalid, -kMaxCoordinate, kMaxCoordinate);
    const int width = settings.intValue(QLatin1String(kWidthKey), kInvalid, kMinWidth, kMaxSize);
    const int height = settings.intValue(QLatin1String(kHeightKey), kInvalid, kMinHeight, kMaxSize);
    if (x == kInvalid || y == kInvalid || width == kInvalid || height == kInvalid) {
        return std::nullopt;
    }

    WindowPlacement placement;
    placement.rect = QRect(x, y, width, height);
    placement.maximized = settings.boolValue(QLatin1String(kMaximizedKey), false);
    return placement;
}

void saveWindowPlacement(SettingsManager& settings, const WindowPlacement& placement)
{
    settings.setInt(QLatin1String(kXKey), placement.rect.x());
    settings.setInt(QLatin1String(kYKey), placement.rect.y());
    settings.setInt(QLatin1String(kWidthKey), placement.rect.width());
    settings.setInt(QLatin1String(kHeightKey), placement.rect.height());
    settings.setBool(QLatin1String(kMaximizedKey), placement.maximized);
}

std::optional<QRect> fitToScreens(const QRect& wanted, const QList<QRect>& screens,
                                  int titleBarHeight)
{
    if (!wanted.isValid() || wanted.isEmpty() || screens.isEmpty()) {
        return std::nullopt;
    }
    titleBarHeight = std::max(titleBarHeight, 0);

    // The title bar is a strip just above the client area; it is what the user grabs.
    const QRect titleBar(wanted.left(), wanted.top() - titleBarHeight, wanted.width(),
                         titleBarHeight);

    // The screen that holds most of the window (its body plus its title bar).
    const QRect* best = nullptr;
    qint64 bestScore = 0;
    for (const QRect& screen : screens) {
        const qint64 score =
            areaOf(screen.intersected(wanted)) + areaOf(screen.intersected(titleBar));
        if (score > bestScore) {
            best = &screen;
            bestScore = score;
        }
    }
    if (best == nullptr) {
        return std::nullopt; // on no screen at all, for example on an unplugged monitor
    }

    // Anything that touches a screen is brought fully into view below: that is more useful to
    // the user than throwing the position away. Shrink to fit the screen (leaving room for the
    // title bar), then move fully inside it.
    const int maxHeight = best->height() - titleBarHeight;
    if (maxHeight <= 0) {
        return std::nullopt;
    }
    const int width = std::min(wanted.width(), best->width());
    const int height = std::min(wanted.height(), maxHeight);
    const int x = std::clamp(wanted.left(), best->left(), best->left() + best->width() - width);
    const int y = std::clamp(wanted.top(), best->top() + titleBarHeight,
                             best->top() + best->height() - height);
    return QRect(x, y, width, height);
}

} // namespace scribe::core
