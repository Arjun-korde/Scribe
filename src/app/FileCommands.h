#pragma once

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QLatin1String>

class DocumentController;
class QWidget;

namespace scribe::app
{
class RecentFilesTracker;
}

/// The File menu commands: new, open, save, save as, exit and clearing the recent files list. Like
/// TabCommands, registering them here keeps the main window free of command details.
class FileCommands
{
    Q_DECLARE_TR_FUNCTIONS(FileCommands)

public:
    // Command ids. Menus refer to these constants, so a typo is a compile error.
    static constexpr QLatin1String kNew{"file.new"};
    static constexpr QLatin1String kOpen{"file.open"};
    static constexpr QLatin1String kSave{"file.save"};
    static constexpr QLatin1String kSaveAs{"file.saveAs"};
    static constexpr QLatin1String kExit{"file.exit"};
    static constexpr QLatin1String kClearRecent{"file.clearRecent"};

    /// Adds the file commands to `registry`. Exit closes `window`, which asks about unsaved
    /// changes in its close event. The commands keep references to `controller` and `window`,
    /// which must outlive every call to registry.run().
    static void registerIn(scribe::core::CommandRegistry& registry, DocumentController& controller,
                           QWidget& window, scribe::app::RecentFilesTracker& recent);
};
