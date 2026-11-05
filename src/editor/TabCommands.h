#pragma once

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QLatin1String>

namespace scribe::editor
{

class EditorTabs;

/// The commands that work on the tab row: close, close others, close all, next and previous.
/// Registering them here keeps the main window free of tab details, and lets tests run them with
/// just a registry and an EditorTabs.
class TabCommands
{
    Q_DECLARE_TR_FUNCTIONS(TabCommands)

public:
    // Command ids. Menus refer to these constants, so a typo is a compile error.
    static constexpr QLatin1String kClose{"tab.close"};
    static constexpr QLatin1String kCloseOthers{"tab.closeOthers"};
    static constexpr QLatin1String kCloseAll{"tab.closeAll"};
    static constexpr QLatin1String kNext{"tab.next"};
    static constexpr QLatin1String kPrevious{"tab.previous"};

    /// Adds the tab commands to `registry`. The commands keep a reference to `tabs`, which must
    /// outlive every call to registry.run().
    static void registerIn(core::CommandRegistry& registry, EditorTabs& tabs);
};

} // namespace scribe::editor
