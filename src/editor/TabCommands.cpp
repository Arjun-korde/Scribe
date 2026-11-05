#include "TabCommands.h"

#include "EditorTabs.h"

namespace scribe::editor
{

void TabCommands::registerIn(core::CommandRegistry& registry, EditorTabs& tabs)
{
    using core::Command;

    // '&' marks the menu mnemonic (Alt+letter).
    registry.registerCommand(Command{kClose, tr("&Close Tab"), QStringLiteral("Ctrl+W"),
                                     [&tabs] { tabs.closeTab(tabs.currentIndex()); }, true});
    registry.registerCommand(Command{kCloseOthers, tr("Close &Other Tabs"), QString(),
                                     [&tabs] { tabs.closeOtherTabs(tabs.currentIndex()); }, true});
    registry.registerCommand(Command{kCloseAll, tr("Close A&ll Tabs"),
                                     QStringLiteral("Ctrl+Shift+W"),
                                     [&tabs] { tabs.closeAllTabs(); }, true});
    registry.registerCommand(Command{kNext, tr("&Next Tab"), QStringLiteral("Ctrl+Tab"),
                                     [&tabs] { tabs.nextTab(); }, true});
    registry.registerCommand(Command{kPrevious, tr("&Previous Tab"),
                                     QStringLiteral("Ctrl+Shift+Tab"),
                                     [&tabs] { tabs.previousTab(); }, true});
}

} // namespace scribe::editor
