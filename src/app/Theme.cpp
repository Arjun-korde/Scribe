#include "Theme.h"

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

    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        p.setColor(QPalette::Disabled, role, textMuted);
    }
    return p;
}

QString Theme::styleSheet() const
{
    // Sizes follow the Figma main window: tab strip 36 px with 32 px tabs, status bar 26 px.
    const QString css = QStringLiteral(R"(
QMenuBar { background: %1; color: %2; }
QMenuBar::item { padding: 4px 8px; background: transparent; }

QTabWidget::pane { border: none; border-top: 1px solid %3; }
QTabBar { background: %1; }
QTabBar::tab {
    height: 32px; min-width: 120px; max-width: 168px;
    padding: 0 8px 0 12px; margin-top: 4px;
    background: transparent; color: %4;
    border: none; border-top-left-radius: 4px; border-top-right-radius: 4px;
}
QTabBar::tab:selected { background: %5; color: %2; }

QPlainTextEdit {
    background: %5; color: %2; border: none;
    selection-background-color: %6; selection-color: %2;
}

QStatusBar { background: %1; color: %4; border-top: 1px solid %3; min-height: 26px; }
QStatusBar::item { border: none; }
QStatusBar QLabel, QStatusBar QToolButton { color: %4; font-size: 12px; }
)");

    const auto hex = [](const QColor& c) { return c.name(QColor::HexRgb); };
    return css.arg(hex(surfaceChrome), hex(textPrimary), hex(outline), hex(textMuted), hex(surface),
                   hex(selection));
}

} // namespace scribe::app
