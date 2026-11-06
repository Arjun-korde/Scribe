#pragma once

#include <QHash>
#include <QString>

#include <functional>
#include <vector>

namespace scribe::core
{

/// One editor action. Menus, shortcuts, the command palette, macros and plugins all invoke
/// actions through their command id instead of calling code directly.
struct Command
{
    QString id;       ///< Stable dotted name such as "file.save". Never shown to the user.
    QString title;    ///< Display text for menus and the palette.
    QString shortcut; ///< Portable text such as "Ctrl+Shift+S", empty for none.
    std::function<void()> callback;

    /// Fixed on/off switch, changed with CommandRegistry::setEnabled().
    bool enabled = true;

    /// Optional live condition, evaluated each time the state is needed, for example "the editor
    /// has a selection". Empty means always satisfied. A command is available only when `enabled`
    /// is true and this condition (if any) holds.
    std::function<bool()> enabledWhen;

    /// Optional live check state for toggles such as "Word Wrap". When set, the command is a
    /// toggle: menus show it with a check mark that follows this function. Empty means an
    /// ordinary command with no check mark.
    std::function<bool()> checkedWhen;

    /// A second shortcut for the same command (for example `Ctrl+=` next to `Ctrl++`), or empty.
    QString alternateShortcut;
};

/// The central list of commands. It has no widget dependency, so the UI layer builds menu
/// entries and shortcuts from it and the logic can be tested without a window.
class CommandRegistry
{
public:
    /// Adds a command. Returns false, changing nothing, when the id is empty, the callback is
    /// empty, or a command with the same id is already registered.
    bool registerCommand(Command command);

    /// The command with this id, or nullptr. The pointer is valid until the next registration.
    const Command* find(const QString& id) const;

    /// Whether the command can run right now: it exists, `enabled` is set and `enabledWhen` (if
    /// any) holds. The fixed switch is checked first, so the condition is not evaluated for a
    /// command that is switched off. Menus and the command palette call this when they open.
    bool isEnabled(const QString& id) const;

    /// Whether a toggle command is currently on. False for an unknown id and for commands that
    /// are not toggles (no `checkedWhen`).
    bool isChecked(const QString& id) const;

    /// Runs the command. Returns false without running anything when the id is unknown or the
    /// command is not enabled (see isEnabled).
    bool run(const QString& id) const;

    /// Returns false when the id is unknown.
    bool setEnabled(const QString& id, bool enabled);

    /// All commands in registration order.
    const std::vector<Command>& commands() const { return m_commands; }

private:
    std::vector<Command> m_commands;
    QHash<QString, std::size_t> m_indexById;
};

} // namespace scribe::core
