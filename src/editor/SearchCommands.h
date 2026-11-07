#pragma once

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QLatin1String>

#include <functional>
#include <optional>

namespace scribe::editor
{

class EditorTabs;

/// The Search menu commands. For now only Go To Line; Find and Replace join it later.
/// They act on the editor of the active tab.
class SearchCommands
{
    Q_DECLARE_TR_FUNCTIONS(SearchCommands)

public:
    // Command ids. Menus refer to these constants, so a typo is a compile error.
    static constexpr QLatin1String kGoToLine{"search.goToLine"};

    /// Asks the user which line to go to, given the line the caret is on and how many lines the
    /// document has. std::nullopt means the user cancelled.
    using LineNumberChooser = std::function<std::optional<int>(int currentLine, int lineCount)>;

    /// Adds the search commands to `registry`. They keep a reference to `tabs`, which must outlive
    /// every call to registry.run() or isEnabled(). Without a `chooseLine` Go To Line is
    /// unavailable.
    static void registerIn(core::CommandRegistry& registry, EditorTabs& tabs,
                           const LineNumberChooser& chooseLine);
};

} // namespace scribe::editor
