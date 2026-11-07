#include "SearchCommands.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

namespace scribe::editor
{

void SearchCommands::registerIn(core::CommandRegistry& registry, EditorTabs& tabs,
                                const LineNumberChooser& chooseLine)
{
    using core::Command;

    // '&' marks the menu mnemonic (Alt+letter).
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
