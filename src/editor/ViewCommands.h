#pragma once

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QFont>
#include <QLatin1String>

#include <functional>
#include <optional>

namespace scribe::editor
{

class EditorTabs;

/// The View menu commands: word wrap, font, zoom in / out / reset, and showing the status bar.
/// Word wrap, font and zoom are shared by all tabs (see ViewSettings).
class ViewCommands
{
    Q_DECLARE_TR_FUNCTIONS(ViewCommands)

public:
    // Command ids. Menus refer to these constants, so a typo is a compile error.
    static constexpr QLatin1String kWordWrap{"view.wordWrap"};
    static constexpr QLatin1String kFont{"view.font"};
    static constexpr QLatin1String kZoomIn{"view.zoomIn"};
    static constexpr QLatin1String kZoomOut{"view.zoomOut"};
    static constexpr QLatin1String kZoomReset{"view.zoomReset"};
    static constexpr QLatin1String kStatusBar{"view.statusBar"};

    /// Asks the user for a font, starting from `current`. std::nullopt means the user cancelled.
    using FontChooser = std::function<std::optional<QFont>(const QFont& current)>;

    /// Adds the view commands to `registry`. The commands keep a reference to `tabs`, which must
    /// outlive every call to registry.run(), isEnabled() or isChecked(). Without a `chooseFont`
    /// the Font command is unavailable. The Status Bar command only changes the setting in `tabs`;
    /// the window shows or hides its status bar when that setting changes.
    static void registerIn(core::CommandRegistry& registry, EditorTabs& tabs,
                           const FontChooser& chooseFont);
};

} // namespace scribe::editor
