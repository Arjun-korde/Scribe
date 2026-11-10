#pragma once

#include <QProxyStyle>

namespace scribe::app
{

/// Paints every menu (the menu bar's drop-downs, the pop-ups next to the status bar, later the
/// context menus) like the Edit menu in the Figma file: a surface panel with a 1 px outline,
/// 28 px rows with 12 px side padding and a 4 px corner radius, the label on the left, the
/// shortcut muted on the right, a `hover` fill under the mouse, and a disabled row at 40% opacity.
///
/// A style sheet cannot do this (it cannot colour the shortcut or fade a row), so it is drawn here.
/// Everything else is left to the underlying Windows style, which also keeps the rounded corners
/// and the shadow of the pop-up window. Colours come from the palette, so a theme change needs
/// nothing here.
class MenuStyle : public QProxyStyle
{
public:
    /// `baseKey` names the style underneath, for example the current application style.
    explicit MenuStyle(const QString& baseKey);

    // The measurements of the design, in logical pixels.
    static constexpr int kRowHeight = 28;
    static constexpr int kRowGap = 2;       ///< Between rows; each row is padded by half of it.
    static constexpr int kPaddingX = 12;    ///< Inside a row, left and right.
    static constexpr int kColumnGap = 16;   ///< Between the label and the shortcut.
    static constexpr int kCheckColumn = 20; ///< Left of the labels when the menu has check marks.
    static constexpr int kPanelPadding = 4; ///< Between the border and the rows.
    static constexpr int kSeparatorRow = 3; ///< A 1 px line with a pixel of padding each side.
    static constexpr int kRowWidth = 250;   ///< The least a row is wide; long labels widen it.
    static constexpr int kCornerRadius = 4;
    static constexpr int kLabelPixelSize = 14;
    static constexpr int kShortcutPixelSize = 13;
    static constexpr qreal kDisabledOpacity = 0.4;

    /// Asks Windows to round the corners of menu pop-ups (as in the design) when they are shown.
    void polish(QWidget* widget) override;
    void unpolish(QWidget* widget) override;

    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize,
                           const QWidget* widget = nullptr) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter,
                     const QWidget* widget = nullptr) const override;

private:
    QObject* m_roundCorners = nullptr; ///< Event filter that rounds a menu's window when shown.
};

} // namespace scribe::app
