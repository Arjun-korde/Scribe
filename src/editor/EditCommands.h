#pragma once

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QLatin1String>
#include <QLocale>

#include <functional>

namespace scribe::editor
{

class EditorTabs;

/// The Edit menu commands: undo, redo, cut, copy, paste, delete, select all, and inserting the
/// date and time. They act on the editor of the active tab.
///
/// Each command reports whether it can run right now (`enabledWhen`): undo needs something to
/// undo, cut, copy and delete need a selection, paste needs text on the clipboard. Menus ask when
/// they open, so there is no state to keep in sync.
class EditCommands
{
    Q_DECLARE_TR_FUNCTIONS(EditCommands)

public:
    // Command ids. Menus refer to these constants, so a typo is a compile error.
    static constexpr QLatin1String kUndo{"edit.undo"};
    static constexpr QLatin1String kRedo{"edit.redo"};
    static constexpr QLatin1String kCut{"edit.cut"};
    static constexpr QLatin1String kCopy{"edit.copy"};
    static constexpr QLatin1String kPaste{"edit.paste"};
    static constexpr QLatin1String kDelete{"edit.delete"};
    static constexpr QLatin1String kSelectAll{"edit.selectAll"};
    static constexpr QLatin1String kInsertDateTime{"edit.insertDateTime"};

    /// The current date and time. Tests replace it so they do not depend on the clock.
    using Clock = std::function<QDateTime()>;

    /// Adds the edit commands to `registry`. The commands keep a reference to `tabs`, which must
    /// outlive every call to registry.run() or registry.isEnabled().
    static void registerIn(
        core::CommandRegistry& registry, EditorTabs& tabs,
        Clock now = [] { return QDateTime::currentDateTime(); });

    /// The text inserted by "Insert Date/Time": the time followed by the date, both in the
    /// short format of `locale` (like Windows Notepad, for example "3:45 PM 11/6/2026").
    static QString timestamp(const QDateTime& when, const QLocale& locale = QLocale::system());
};

} // namespace scribe::editor
