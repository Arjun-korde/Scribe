#include "CardDialog.h"

#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QWindow>

namespace scribe::app
{

namespace
{

/// Paints the card: the surface colour inside a 1 px outline with rounded corners.
class Card : public QWidget
{
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // The outline is centred on the path, so the path is half a pixel inside the card.
        const qreal inset = CardDialog::kBorderWidth / 2.0;
        QPainterPath shape;
        shape.addRoundedRect(QRectF(rect()).adjusted(inset, inset, -inset, -inset),
                             CardDialog::kCornerRadius, CardDialog::kCornerRadius);

        painter.fillPath(shape, palette().color(QPalette::Base));
        painter.setPen(QPen(palette().color(QPalette::Mid), CardDialog::kBorderWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shape);
    }
};

} // namespace

CardDialog::CardDialog(QWidget* parent)
    : QDialog(parent)
{
    // No system title bar and a see-through window, so the card's rounded corners and its shadow
    // can be drawn. The window title is still set by the derived dialog (taskbar, screen readers).
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_card = new Card(this);
    auto* shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(kShadowBlur);
    shadow->setOffset(0, kShadowOffsetY);
    shadow->setColor(QColor(0, 0, 0, qRound(255 * kShadowAlpha)));
    m_card->setGraphicsEffect(shadow);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(kShadowMargin, kShadowMargin, kShadowMargin, kShadowMargin);
    outer->addWidget(m_card);

    m_layout = new QVBoxLayout(m_card);
    m_layout->setSpacing(0);
}

void CardDialog::mousePressEvent(QMouseEvent* event)
{
    // A press that reaches the dialog itself (not a button or other control) starts a drag, since
    // the card has no title bar to drag it by.
    if (event->button() == Qt::LeftButton && windowHandle() != nullptr) {
        windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

} // namespace scribe::app
