#include "Theme.h"

#include <QList>

#include <utility>

namespace scribe::app
{

Theme Theme::light()
{
    return {
        .name = QStringLiteral("Light"),
        .surface = QColor(0xff, 0xff, 0xff),
        .surfaceChrome = QColor(0xf3, 0xf3, 0xf3),
        .textPrimary = QColor(0x1b, 0x1b, 0x1b),
        .textMuted = QColor(0x5c, 0x5c, 0x5c),
        .outline = QColor(0xe0, 0xe0, 0xe0),
        .currentLine = QColor(0xf5, 0xf5, 0xf5),
        .selection = QColor(0xcc, 0xe4, 0xf7),
        .accent = QColor(0x0f, 0x6c, 0xbd),
        .hover = QColor(0xe9, 0xe9, 0xe9),
        .pressed = QColor(0xdd, 0xdd, 0xdd),
    };
}

Theme Theme::dark()
{
    return {
        .name = QStringLiteral("Dark"),
        .surface = QColor(0x1e, 0x1e, 0x1e),
        .surfaceChrome = QColor(0x2b, 0x2b, 0x2b),
        .textPrimary = QColor(0xe6, 0xe6, 0xe6),
        .textMuted = QColor(0xa0, 0xa0, 0xa0),
        .outline = QColor(0x3a, 0x3a, 0x3a),
        .currentLine = QColor(0x26, 0x26, 0x26),
        .selection = QColor(0x26, 0x4f, 0x78),
        .accent = QColor(0x60, 0xcd, 0xff),
        .hover = QColor(0x35, 0x35, 0x35),
        .pressed = QColor(0x44, 0x44, 0x44),
    };
}

QPalette Theme::palette() const
{
    QPalette p;
    p.setColor(QPalette::Window, surfaceChrome);
    p.setColor(QPalette::WindowText, textPrimary);
    p.setColor(QPalette::Base, surface);
    p.setColor(QPalette::AlternateBase, currentLine);
    p.setColor(QPalette::Text, textPrimary);
    p.setColor(QPalette::PlaceholderText, textMuted);
    p.setColor(QPalette::Button, surfaceChrome);
    p.setColor(QPalette::ButtonText, textPrimary);
    p.setColor(QPalette::ToolTipBase, surfaceChrome);
    p.setColor(QPalette::ToolTipText, textPrimary);
    p.setColor(QPalette::Highlight, selection);
    // Selected text keeps its colour, so it never sits on a clashing fill.
    p.setColor(QPalette::HighlightedText, textPrimary);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::Mid, outline);
    p.setColor(QPalette::Midlight, hover); // the tab bar paints its own hovered tab with this

    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        p.setColor(QPalette::Disabled, role, textMuted);
    }
    return p;
}

QString Theme::styleSheet() const
{
    // Sizes follow the Figma main window: tab strip 36 px with 32 px tabs, status bar 26 px.
    // @name@ is replaced by the theme's colour of that name.
    QString css = QStringLiteral(R"(
QMenuBar { background: @surfaceChrome@; color: @textPrimary@; font-size: 14px; }
QMenuBar::item { padding: 3px 8px; margin: 4px 0; background: transparent; border-radius: 4px; }
QMenuBar::item:selected { background: @hover@; }
QMenuBar::item:pressed { background: @pressed@; }

QTabWidget::pane { border: none; border-top: 1px solid @outline@; }
QToolButton#newTabButton, QToolButton#tabCloseButton {
    border: none; border-radius: 4px; background: transparent; color: @textMuted@;
}
QToolButton#newTabButton { font-size: 16px; }
QToolButton#tabCloseButton { font-size: 14px; }
QToolButton#newTabButton:hover, QToolButton#tabCloseButton:hover { background: @hover@; }
QToolButton#newTabButton:pressed, QToolButton#tabCloseButton:pressed { background: @pressed@; }

QPlainTextEdit { color: @textPrimary@; border: none;
    selection-background-color: @selection@; selection-color: @textPrimary@; }

QStatusBar { background: @surfaceChrome@; color: @textMuted@; border-top: 1px solid @outline@;
    min-height: 26px; }
QStatusBar::item { border: none; }
QStatusBar QLabel, QStatusBar QToolButton { color: @textMuted@; font-size: 12px; }
QStatusBar QToolButton { border: none; border-radius: 4px; padding: 2px 4px; }
QStatusBar QToolButton:hover { background: @hover@; }
QStatusBar QToolButton:pressed { background: @pressed@; }
)");

    const QList<std::pair<QString, QColor>> tokens = {
        {QStringLiteral("surfaceChrome"), surfaceChrome},
        {QStringLiteral("surface"), surface},
        {QStringLiteral("textPrimary"), textPrimary},
        {QStringLiteral("textMuted"), textMuted},
        {QStringLiteral("outline"), outline},
        {QStringLiteral("selection"), selection},
        {QStringLiteral("hover"), hover},
        {QStringLiteral("pressed"), pressed},
    };
    for (const auto& [token, color] : tokens) {
        css.replace(QLatin1Char('@') + token + QLatin1Char('@'), color.name(QColor::HexRgb));
    }
    return css;
}

} // namespace scribe::app
