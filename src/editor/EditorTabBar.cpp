#include "EditorTabBar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolButton>

namespace scribe::editor
{

namespace
{
constexpr int kCornerRadius = 4;
constexpr int kTextPaddingLeft = 12;
constexpr int kCloseButtonSize = 20;
constexpr int kCloseButtonPaddingRight = 8;
constexpr int kTextPixelSize = 13;
} // namespace

EditorTabBar::EditorTabBar(QWidget* parent)
    : QTabBar(parent)
{
    setDrawBase(false);
    setExpanding(false);
    setElideMode(Qt::ElideRight);
    setMouseTracking(true);
}

QSize EditorTabBar::tabSizeHint(int index) const
{
    return {(index == 0 ? kLeadIn : 0) + kTabWidth + kTabSpacing, kTopGap + kTabHeight};
}

QRect EditorTabBar::bodyRect(int index) const
{
    const QRect full = tabRect(index);
    return {full.left() + (index == 0 ? kLeadIn : 0), full.top() + kTopGap, kTabWidth, kTabHeight};
}

void EditorTabBar::tabInserted(int index)
{
    QTabBar::tabInserted(index);

    auto* close = new QToolButton(this);
    close->setObjectName(QStringLiteral("tabCloseButton")); // styled by the theme
    close->setText(QStringLiteral("×"));
    close->setToolTip(tr("Close"));
    close->setAccessibleName(tr("Close tab"));
    close->setFocusPolicy(Qt::NoFocus);
    close->setFixedSize(kCloseButtonSize, kCloseButtonSize);
    // The tab can have moved since the button was made, so look its index up when it is clicked.
    connect(close, &QToolButton::clicked, this, [this, close] {
        const int tab = tabAt(close->geometry().center());
        if (tab >= 0) {
            emit tabCloseRequested(tab);
        }
    });
    setTabButton(index, QTabBar::RightSide, close);
}

void EditorTabBar::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Window));

    QFont labelFont = font();
    labelFont.setPixelSize(kTextPixelSize);
    painter.setFont(labelFont);

    for (int i = 0; i < count(); ++i) {
        const QRect body = bodyRect(i);

        const bool selected = i == currentIndex();
        if (selected || i == m_hoveredTab) {
            // Selected tabs share the editor's colour, so the tab and the page read as one.
            const QColor fill = palette().color(selected ? QPalette::Base : QPalette::Midlight);
            constexpr qreal r = kCornerRadius;
            const QRectF b(body);
            QPainterPath shape; // rounded at the top, square at the bottom where it meets the page
            shape.moveTo(b.left(), b.bottom());
            shape.lineTo(b.left(), b.top() + r);
            shape.arcTo(QRectF(b.left(), b.top(), 2 * r, 2 * r), 180, -90);
            shape.lineTo(b.right() - r, b.top());
            shape.arcTo(QRectF(b.right() - 2 * r, b.top(), 2 * r, 2 * r), 90, -90);
            shape.lineTo(b.right(), b.bottom());
            shape.closeSubpath();
            painter.fillPath(shape, fill);
        }

        const int textRight = body.right() - kCloseButtonPaddingRight - kCloseButtonSize;
        const QRect text(body.left() + kTextPaddingLeft, body.top(),
                         textRight - body.left() - kTextPaddingLeft, body.height());
        painter.setPen(palette().color(selected ? QPalette::Text : QPalette::PlaceholderText));
        painter.drawText(
            text, Qt::AlignLeft | Qt::AlignVCenter,
            painter.fontMetrics().elidedText(tabText(i), Qt::ElideRight, text.width()));
    }
}

void EditorTabBar::tabLayoutChange()
{
    QTabBar::tabLayoutChange();

    // QTabBar has just put each button where its style likes it; move it into the tab's padding.
    for (int i = 0; i < count(); ++i) {
        if (QWidget* close = tabButton(i, QTabBar::RightSide)) {
            const QRect body = bodyRect(i);
            close->move(body.right() + 1 - kCloseButtonPaddingRight - kCloseButtonSize,
                        body.top() + (kTabHeight - kCloseButtonSize) / 2);
        }
    }
}

void EditorTabBar::mouseMoveEvent(QMouseEvent* event)
{
    QTabBar::mouseMoveEvent(event);
    const int hovered = tabAt(event->position().toPoint());
    if (hovered != m_hoveredTab) {
        m_hoveredTab = hovered;
        update();
    }
}

void EditorTabBar::leaveEvent(QEvent* event)
{
    QTabBar::leaveEvent(event);
    m_hoveredTab = -1;
    update();
}

} // namespace scribe::editor
