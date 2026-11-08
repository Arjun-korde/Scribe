#include "SearchCommands.h"

#include "EditorTabs.h"
#include "EditorWidget.h"
#include "FindBar.h"

namespace scribe::editor
{

void SearchCommands::registerIn(core::CommandRegistry& registry, EditorTabs& tabs, FindBar& findBar,
                                const LineNumberChooser& chooseLine)
{
    using core::Command;

    // '&' marks the menu mnemonic (Alt+letter).
    registry.registerCommand(Command{
        .id = kFind,
        .title = tr("&Find..."),
        .shortcut = QStringLiteral("Ctrl+F"),
        .callback = [&findBar] { findBar.openFind(); },
        .enabled = true,
        .enabledWhen = {},
        .checkedWhen = {},
        .alternateShortcut = {},
    });
    registry.registerCommand(Command{
        .id = kFindNext,
        .title = tr("Find &Next"),
        .shortcut = QStringLiteral("F3"),
        .callback = [&findBar] { findBar.findNext(); },
        .enabled = true,
        .enabledWhen = {},
        .checkedWhen = {},
        .alternateShortcut = {},
    });
    registry.registerCommand(Command{
        .id = kFindPrevious,
        .title = tr("Find &Previous"),
        .shortcut = QStringLiteral("Shift+F3"),
        .callback = [&findBar] { findBar.findPrevious(); },
        .enabled = true,
        .enabledWhen = {},
        .checkedWhen = {},
        .alternateShortcut = {},
    });
    registry.registerCommand(Command{
        .id = kReplace,
        .title = tr("&Replace..."),
        .shortcut = QStringLiteral("Ctrl+H"),
        .callback = [&findBar] { findBar.openReplace(); },
        .enabled = true,
        .enabledWhen = {},
        .checkedWhen = {},
        .alternateShortcut = {},
    });
    registry.registerCommand(Command{
        .id = kReplaceNext,
        .title = tr("Replace Next"),
        .shortcut = {},
        .callback = [&findBar] { findBar.replaceCurrent(); },
        .enabled = true,
        .enabledWhen = [&findBar] { return findBar.hasQuery(); },
        .checkedWhen = {},
        .alternateShortcut = {},
    });
    registry.registerCommand(Command{
        .id = kReplaceAll,
        .title = tr("Replace &All"),
        .shortcut = {},
        .callback = [&findBar] { findBar.replaceAll(); },
        .enabled = true,
        .enabledWhen = [&findBar] { return findBar.hasQuery(); },
        .checkedWhen = {},
        .alternateShortcut = {},
    });

    registry.registerCommand(Command{
        .id = kGoToLine,
        .title = tr("&Go To Line..."),
        .shortcut = QStringLiteral("Ctrl+G"),
        .callback =
            [&tabs, chooseLine] {
                EditorWidget* editor = tabs.currentEditor();
                if (const std::optional<int> line =
                        chooseLine(editor->currentLine(), editor->lineCount())) {
                    editor->goToLine(*line);
                }
            },
        .enabled = true,
        .enabledWhen = [chooseLine] { return static_cast<bool>(chooseLine); },
        .checkedWhen = {},
        .alternateShortcut = {},
    });
}

} // namespace scribe::editor
