#include "FileCommands.h"

#include "DocumentController.h"

#include <QWidget>

void FileCommands::registerIn(scribe::core::CommandRegistry& registry,
                              DocumentController& controller, QWidget& window)
{
    using scribe::core::Command;

    // '&' marks the menu mnemonic (Alt+letter). Exit has no shortcut: Alt+F4 is handled by Windows.
    registry.registerCommand(Command{kNew, tr("&New"), QStringLiteral("Ctrl+N"),
                                     [&controller] { controller.newFile(); }, true});
    registry.registerCommand(Command{kOpen, tr("&Open..."), QStringLiteral("Ctrl+O"),
                                     [&controller] { controller.openFile(); }, true});
    registry.registerCommand(Command{kSave, tr("&Save"), QStringLiteral("Ctrl+S"),
                                     [&controller] { controller.saveFile(); }, true});
    registry.registerCommand(Command{kSaveAs, tr("Save &As..."), QStringLiteral("Ctrl+Shift+S"),
                                     [&controller] { controller.saveFileAs(); }, true});
    registry.registerCommand(
        Command{kExit, tr("E&xit"), QString(), [&window] { window.close(); }, true});
}
