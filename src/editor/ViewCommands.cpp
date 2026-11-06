#include "ViewCommands.h"

#include "EditorTabs.h"

#include <QStatusBar>

namespace scribe::editor
{

void ViewCommands::registerIn(core::CommandRegistry& registry, EditorTabs& tabs,
                              QStatusBar& statusBar, const FontChooser& chooseFont)
{
    using core::Command;

    // '&' marks the menu mnemonic (Alt+letter).
    registry.registerCommand(Command{
        .id = kWordWrap,
        .title = tr("Word &Wrap"),
        .shortcut = {},
        .callback = [&tabs] { tabs.setWordWrap(!tabs.viewSettings().wordWrap); },
        .enabled = true,
        .enabledWhen = {},
        .checkedWhen = [&tabs] { return tabs.viewSettings().wordWrap; },
        .alternateShortcut = {},
    });

    registry.registerCommand(Command{
        .id = kFont,
        .title = tr("&Font..."),
        .shortcut = {},
        .callback =
            [&tabs, chooseFont] {
                // The dialog starts from the base font, not the zoomed one, so the zoom is not
                // applied twice.
                if (const std::optional<QFont> font = chooseFont(tabs.viewSettings().font)) {
                    tabs.setBaseFont(*font);
                }
            },
        .enabled = true,
        .enabledWhen = [chooseFont] { return static_cast<bool>(chooseFont); },
        .checkedWhen = {},
        .alternateShortcut = {},
    });

    // "Ctrl++" needs Shift on most keyboards, so "Ctrl+=" works too, as in browsers and Notepad.
    registry.registerCommand(Command{
        .id = kZoomIn,
        .title = tr("Zoom &In"),
        .shortcut = QStringLiteral("Ctrl++"),
        .callback = [&tabs] { tabs.zoomBy(1); },
        .enabled = true,
        .enabledWhen =
            [&tabs] { return tabs.viewSettings().zoomPercent < ViewSettings::kMaxZoomPercent; },
        .checkedWhen = {},
        .alternateShortcut = QStringLiteral("Ctrl+="),
    });
    registry.registerCommand(Command{
        .id = kZoomOut,
        .title = tr("Zoom &Out"),
        .shortcut = QStringLiteral("Ctrl+-"),
        .callback = [&tabs] { tabs.zoomBy(-1); },
        .enabled = true,
        .enabledWhen =
            [&tabs] { return tabs.viewSettings().zoomPercent > ViewSettings::kMinZoomPercent; },
        .checkedWhen = {},
        .alternateShortcut = {},
    });
    registry.registerCommand(Command{
        .id = kZoomReset,
        .title = tr("&Restore Default Zoom"),
        .shortcut = QStringLiteral("Ctrl+0"),
        .callback = [&tabs] { tabs.resetZoom(); },
        .enabled = true,
        .enabledWhen =
            [&tabs] {
                return tabs.viewSettings().zoomPercent != ViewSettings::kDefaultZoomPercent;
            },
        .checkedWhen = {},
        .alternateShortcut = {},
    });

    registry.registerCommand(Command{
        .id = kStatusBar,
        .title = tr("&Status Bar"),
        .shortcut = {},
        // isHidden() is the explicit "hide" request, unlike isVisible() which is also false while
        // the whole window is not on screen.
        .callback = [&statusBar] { statusBar.setVisible(statusBar.isHidden()); },
        .enabled = true,
        .enabledWhen = {},
        .checkedWhen = [&statusBar] { return !statusBar.isHidden(); },
        .alternateShortcut = {},
    });
}

} // namespace scribe::editor
