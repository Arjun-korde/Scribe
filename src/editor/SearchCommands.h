#pragma once

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QLatin1String>

#include <functional>
#include <optional>

namespace scribe::editor
{

class EditorTabs;
class FindBar;

/// The Search menu commands: find, find next / previous, replace, and go to line. They act on the
/// editor of the active tab, through the window's find bar.
class SearchCommands
{
    Q_DECLARE_TR_FUNCTIONS(SearchCommands)

public:
    // Command ids. Menus refer to these constants, so a typo is a compile error.
    static constexpr QLatin1String kFind{"search.find"};
    static constexpr QLatin1String kFindNext{"search.findNext"};
    static constexpr QLatin1String kFindPrevious{"search.findPrevious"};
    static constexpr QLatin1String kReplace{"search.replace"};
    static constexpr QLatin1String kReplaceNext{"search.replaceNext"};
    static constexpr QLatin1String kReplaceAll{"search.replaceAll"};
    static constexpr QLatin1String kGoToLine{"search.goToLine"};

    /// Asks the user which line to go to, given the line the caret is on and how many lines the
    /// document has. std::nullopt means the user cancelled.
    using LineNumberChooser = std::function<std::optional<int>(int currentLine, int lineCount)>;

    /// Adds the search commands to `registry`. They keep references to `tabs` and `findBar`, which
    /// must outlive every call to registry.run() or isEnabled(). Without a `chooseLine` Go To Line
    /// is unavailable.
    ///
    /// Replace Next and Replace All (the bar's two buttons) are available only while the bar has
    /// text to find. They are not in the menu; they exist so that the command palette and macros
    /// can use them.
    static void registerIn(core::CommandRegistry& registry, EditorTabs& tabs, FindBar& findBar,
                           const LineNumberChooser& chooseLine);
};

} // namespace scribe::editor
