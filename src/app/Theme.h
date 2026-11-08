#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

namespace scribe::app
{

/// The colour tokens of one theme, named like the variables in the Figma file ("Scribe UI Design").
/// A theme is only data: `palette()` and `styleSheet()` turn it into what Qt draws with, so a user
/// theme later needs no code changes.
struct Theme
{
    QString name;
    QColor surface;       ///< The editor and the selected tab.
    QColor surfaceChrome; ///< Title bar, tab strip, status bar and menus.
    QColor textPrimary;
    QColor textMuted;
    QColor outline;     ///< 1 px dividers.
    QColor currentLine; ///< Background of the line with the cursor.
    QColor selection;   ///< Background of selected text.
    QColor accent;
    QColor hover;   ///< Background of a chrome control under the mouse.
    QColor pressed; ///< Background of a chrome control while the mouse button is down.

    static Theme light();
    static Theme dark();

    /// Maps the tokens onto the palette roles, so standard widgets and dialogs follow the theme.
    QPalette palette() const;

    /// Styles the parts the palette cannot: the tab strip, status bar, menu bar and editor.
    QString styleSheet() const;
};

} // namespace scribe::app
