#include "CommandRegistry.h"

namespace scribe::core
{

bool CommandRegistry::registerCommand(Command command)
{
    if (command.id.isEmpty() || !command.callback || m_indexById.contains(command.id)) {
        return false;
    }
    m_indexById.insert(command.id, m_commands.size());
    m_commands.push_back(std::move(command));
    return true;
}

const Command* CommandRegistry::find(const QString& id) const
{
    const auto it = m_indexById.constFind(id);
    return it == m_indexById.constEnd() ? nullptr : &m_commands[it.value()];
}

bool CommandRegistry::run(const QString& id) const
{
    const Command* command = find(id);
    if (command == nullptr || !command->enabled) {
        return false;
    }
    command->callback();
    return true;
}

bool CommandRegistry::setEnabled(const QString& id, bool enabled)
{
    const auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) {
        return false;
    }
    m_commands[it.value()].enabled = enabled;
    return true;
}

} // namespace scribe::core
