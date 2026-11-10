#pragma once

#include <QDialog>

class QVBoxLayout;

namespace scribe::app
{

/// The frame shared by the dialogs of the design (unsaved changes, go to line, about, ...): a
/// borderless card with a 1 px outline, an 8 px corner radius and a soft shadow, without a system
/// title bar. The dialog's own title is a label inside the card.
///
/// Derived dialogs put their widgets into cardLayout(). The card's colours come from the palette,
/// so the theme applies. Esc closes the dialog like Cancel does (QDialog's default), and the
/// card can be dragged by any part that is not a control.
class CardDialog : public QDialog
{
    Q_OBJECT

public:
    static constexpr int kCornerRadius = 8;
    static constexpr int kBorderWidth = 1;
    static constexpr int kShadowBlur = 24;
    static constexpr int kShadowOffsetY = 8;
    static constexpr qreal kShadowAlpha = 0.28;
    /// Room around the card for its shadow, inside the dialog's window.
    static constexpr int kShadowMargin = 32;

    explicit CardDialog(QWidget* parent = nullptr);

    /// The card itself, 1 px smaller than the window's content on each side by the shadow margin.
    QWidget* card() const { return m_card; }

    /// The layout inside the card. Its margins include the border, so widgets are placed with the
    /// design's offsets from the card's edge (25 px = 1 px border + 24 px padding).
    QVBoxLayout* cardLayout() const { return m_layout; }

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QWidget* m_card = nullptr;
    QVBoxLayout* m_layout = nullptr;
};

} // namespace scribe::app
