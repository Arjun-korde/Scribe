#include "EditCommands.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QTextCursor>
#include <QTextDocument>

namespace scribe::editor
{

QString EditCommands::timestamp(const QDateTime& when, const QLocale& locale)
{
    return locale.toString(when.time(), QLocale::ShortFormat) + QLatin1Char(' ') +
           locale.toString(when.date(), QLocale::ShortFormat);
}

void EditCommands::registerIn(core::CommandRegistry& registry, EditorTabs& tabs, Clock now)
{
    using core::Command;

    const auto editor = [&tabs] { return tabs.currentEditor(); };
    const auto hasSelection = [editor] {
        return editor()->textCursor().hasSelection() && !editor()->isReadOnly();
    };

    // '&' marks the menu mnemonic (Alt+letter). Ctrl+Z, Ctrl+X, Ctrl+C, Ctrl+V, Ctrl+A, Ctrl+Y and
    // Delete are also handled by the text editor itself while it has focus; the shortcuts here are
    // what the menu displays.
    registry.registerCommand(Command{
        .id = kUndo,
        .title = tr("&Undo"),
        .shortcut = QStringLiteral("Ctrl+Z"),
        .callback = [editor] { editor()->undo(); },
        .enabled = true,
        .enabledWhen = [editor] { return editor()->document()->isUndoAvailable(); },
    });
    registry.registerCommand(Command{
        .id = kRedo,
        .title = tr("&Redo"),
        .shortcut = QStringLiteral("Ctrl+Y"),
        .callback = [editor] { editor()->redo(); },
        .enabled = true,
        .enabledWhen = [editor] { return editor()->document()->isRedoAvailable(); },
    });
    registry.registerCommand(Command{
        .id = kCut,
        .title = tr("Cu&t"),
        .shortcut = QStringLiteral("Ctrl+X"),
        .callback = [editor] { editor()->cut(); },
        .enabled = true,
        .enabledWhen = hasSelection,
    });
    registry.registerCommand(Command{
        .id = kCopy,
        .title = tr("&Copy"),
        .shortcut = QStringLiteral("Ctrl+C"),
        .callback = [editor] { editor()->copy(); },
        .enabled = true,
        .enabledWhen = [editor] { return editor()->textCursor().hasSelection(); },
    });
    registry.registerCommand(Command{
        .id = kPaste,
        .title = tr("&Paste"),
        .shortcut = QStringLiteral("Ctrl+V"),
        .callback = [editor] { editor()->paste(); },
        .enabled = true,
        .enabledWhen = [editor] { return editor()->canPaste(); },
    });
    registry.registerCommand(Command{
        .id = kDelete,
        .title = tr("De&lete"),
        .shortcut = QStringLiteral("Delete"),
        .callback =
            [editor] {
                // Like Notepad's menu item: removes the selection, and does nothing without one.
                QTextCursor cursor = editor()->textCursor();
                cursor.removeSelectedText();
                editor()->setTextCursor(cursor);
            },
        .enabled = true,
        .enabledWhen = hasSelection,
    });
    registry.registerCommand(Command{
        .id = kSelectAll,
        .title = tr("Select &All"),
        .shortcut = QStringLiteral("Ctrl+A"),
        .callback = [editor] { editor()->selectAll(); },
        .enabled = true,
        .enabledWhen = {},
    });
    registry.registerCommand(Command{
        .id = kInsertDateTime,
        .title = tr("Insert &Date/Time"),
        .shortcut = QStringLiteral("F5"),
        .callback =
            [editor, now = std::move(now)] {
                // An edit block keeps the stamp a separate undo step. Without it Qt merges the
                // insertion with the text typed just before, and Ctrl+Z would remove both.
                QTextCursor cursor = editor()->textCursor();
                cursor.beginEditBlock();
                cursor.insertText(timestamp(now()));
                cursor.endEditBlock();
                editor()->setTextCursor(cursor);
            },
        .enabled = true,
        .enabledWhen = [editor] { return !editor()->isReadOnly(); },
    });
}

} // namespace scribe::editor
