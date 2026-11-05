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
    bool enabled = true;
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

    /// Runs the command. Returns false without running anything when the id is unknown or the
    /// command is disabled.
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
