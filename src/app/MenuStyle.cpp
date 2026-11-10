#include "MenuStyle.h"

#include <QAction>
#include <QEvent>
#include <QFontMetrics>
#include <QKeySequence>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionMenuItem>

#include <algorithm>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX // windows.h would otherwise define min and max macros that break std::max
#endif
#include <dwmapi.h>
#include <windows.h>
#endif

namespace scribe::app
{

namespace
{

QFont withPixelSize(QFont font, int pixelSize)
{
    font.setPixelSize(pixelSize);
    return font;
}

/// The label without its mnemonic marker: "&File" is shown as "File", and "&&" is a real "&".
QString shownLabel(const QString& label)
{
    QString text = label;
    text.replace(QStringLiteral("&&"), QStringLiteral("\u0001"));
    text.remove(QLatin1Char('&'));
    text.replace(QLatin1Char('\u0001'), QLatin1Char('&'));
    return text;
}

/// The width QMenu adds to every row for its shortcut column: the widest shortcut of the menu, in
/// the menu's own font. QMenu does this itself, after asking the style for the size of each row,
/// and it does it whatever the style draws. This style already counts the shortcut in a row's
/// width, so it takes this amount off again; otherwise rows with shortcuts would come out wider
/// than the rows without. The rule is the one in QMenuPrivate::updateActionRects().
int shortcutColumnWidth(const QWidget* widget)
{
    const auto* menu = qobject_cast<const QMenu*>(widget);
    if (menu == nullptr) {
        return 0;
    }

    const QFontMetrics metrics = menu->fontMetrics();
    int widest = 0;
    for (const QAction* action : menu->actions()) {
        if (action->isSeparator()) {
            continue;
        }
        const QString text = action->text();
        const qsizetype tab = text.indexOf(QLatin1Char('\t'));
        if (tab != -1) {
            widest = std::max(widest, metrics.horizontalAdvance(text.mid(tab + 1)));
        } else if (!action->shortcut().isEmpty()) {
            widest = std::max(widest, metrics.horizontalAdvance(
                                          action->shortcut().toString(QKeySequence::NativeText)));
        }
    }
    return widest;
}

/// Menu text is "label<TAB>shortcut"; the shortcut part may be missing.
QString labelOf(const QStyleOptionMenuItem& item)
{
    return item.text.section(QLatin1Char('\t'), 0, 0);
}

QString shortcutOf(const QStyleOptionMenuItem& item)
{
    return item.text.section(QLatin1Char('\t'), 1);
}

bool hasShortcut(const QStyleOptionMenuItem& item)
{
    return item.text.contains(QLatin1Char('\t'));
}

void drawCheckMark(QPainter& painter, const QPointF& centre, const QColor& colour)
{
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(colour, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(
        QPolygonF{centre + QPointF(-4, 0.5), centre + QPointF(-1.5, 3), centre + QPointF(4, -3)});
}

void drawSubmenuArrow(QPainter& painter, const QPointF& centre, const QColor& colour)
{
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(colour, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(
        QPolygonF{centre + QPointF(-2, -4), centre + QPointF(2, 0), centre + QPointF(-2, 4)});
}

} // namespace

namespace
{

/// Rounds the window of whatever it watches, each time it is shown (the window handle can be
/// created again for every pop-up).
class RoundCorners : public QObject
{
public:
    using QObject::QObject;

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Show) {
#ifdef Q_OS_WIN
            if (auto* widget = qobject_cast<QWidget*>(watched); widget && widget->isWindow()) {
                // DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_ROUND = 2. Windows 10 ignores it.
                const DWORD preference = 2;
                DwmSetWindowAttribute(reinterpret_cast<HWND>(widget->winId()), 33, &preference,
                                      sizeof(preference));
            }
#endif
        }
        return QObject::eventFilter(watched, event);
    }
};

} // namespace

MenuStyle::MenuStyle(const QString& baseKey)
    : QProxyStyle(baseKey)
    , m_roundCorners(new RoundCorners(this))
{}

void MenuStyle::polish(QWidget* widget)
{
    QProxyStyle::polish(widget);
    if (qobject_cast<QMenu*>(widget) != nullptr) {
        widget->installEventFilter(m_roundCorners);
    }
}

void MenuStyle::unpolish(QWidget* widget)
{
    if (qobject_cast<QMenu*>(widget) != nullptr) {
        widget->removeEventFilter(m_roundCorners);
    }
    QProxyStyle::unpolish(widget);
}

int MenuStyle::pixelMetric(PixelMetric metric, const QStyleOption* option,
                           const QWidget* widget) const
{
    switch (metric) {
        case PM_MenuPanelWidth:
            return 1; // the outline
        case PM_MenuHMargin:
            return kPanelPadding;
        case PM_MenuVMargin:
            // Each row is padded by half the gap, so the first and last rows need that much less.
            return kPanelPadding - kRowGap / 2;
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QSize MenuStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                  const QSize& contentsSize, const QWidget* widget) const
{
    const auto* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
    if (type != CT_MenuItem || item == nullptr) {
        return QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    }

    const int columnQMenuAdds = shortcutColumnWidth(widget);

    if (item->menuItemType == QStyleOptionMenuItem::Separator) {
        return {kRowWidth - columnQMenuAdds, kSeparatorRow};
    }

    const QFontMetrics labelMetrics(withPixelSize(item->font, kLabelPixelSize));
    const QFontMetrics shortcutMetrics(withPixelSize(item->font, kShortcutPixelSize));

    int width = 2 * kPaddingX + labelMetrics.horizontalAdvance(shownLabel(labelOf(*item)));
    if (item->menuHasCheckableItems) {
        width += kCheckColumn;
    }
    if (hasShortcut(*item)) {
        width += kColumnGap + shortcutMetrics.horizontalAdvance(shortcutOf(*item));
    } else if (item->menuItemType == QStyleOptionMenuItem::SubMenu) {
        width += kColumnGap + 8; // room for the arrow
    }
    return {std::max(width, kRowWidth) - columnQMenuAdds, kRowHeight + kRowGap};
}

void MenuStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                              QPainter* painter, const QWidget* widget) const
{
    if (element == PE_PanelMenu) {
        painter->fillRect(option->rect, option->palette.color(QPalette::Base));
        return;
    }
    if (element == PE_FrameMenu) {
        painter->save();
        painter->setPen(option->palette.color(QPalette::Mid));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore();
        return;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void MenuStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter,
                            const QWidget* widget) const
{
    if (element == CE_MenuEmptyArea) {
        painter->fillRect(option->rect, option->palette.color(QPalette::Base));
        return;
    }

    const auto* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
    if (element != CE_MenuItem || item == nullptr) {
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    const QPalette& colours = item->palette;
    using Type = QStyleOptionMenuItem;

    if (item->menuItemType == Type::Separator) {
        painter->fillRect(QRect(item->rect.left(), item->rect.top() + 1, item->rect.width(), 1),
                          colours.color(QPalette::Mid));
        return;
    }
    if (item->menuItemType != Type::Normal && item->menuItemType != Type::DefaultItem &&
        item->menuItemType != Type::SubMenu) {
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    painter->save();
    const bool enabled = item->state & State_Enabled;
    if (!enabled) {
        painter->setOpacity(kDisabledOpacity);
    }

    const QRect row = item->rect.adjusted(0, kRowGap / 2, 0, -kRowGap / 2);
    if (enabled && (item->state & State_Selected)) {
        QPainterPath shape;
        shape.addRoundedRect(row, kCornerRadius, kCornerRadius);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(shape, colours.color(QPalette::Midlight));
    }

    const QColor text = colours.color(QPalette::Text);
    const QColor muted = colours.color(QPalette::PlaceholderText);

    int left = row.left() + kPaddingX;
    if (item->menuHasCheckableItems) {
        if (item->checked) {
            drawCheckMark(*painter, QPointF(left + 5, row.center().y() + 0.5), text);
        }
        left += kCheckColumn;
    }

    const int mnemonic = proxy()->styleHint(SH_UnderlineShortcut, option, widget)
                             ? Qt::TextShowMnemonic
                             : Qt::TextHideMnemonic;
    const int flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | mnemonic;

    painter->setFont(withPixelSize(item->font, kLabelPixelSize));
    painter->setPen(text);
    painter->drawText(QRect(left, row.top(), row.right() - kPaddingX - left, row.height()), flags,
                      labelOf(*item));

    if (hasShortcut(*item)) {
        painter->setFont(withPixelSize(item->font, kShortcutPixelSize));
        painter->setPen(muted);
        painter->drawText(QRect(left, row.top(), row.right() + 1 - kPaddingX - left, row.height()),
                          Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine,
                          shortcutOf(*item));
    } else if (item->menuItemType == Type::SubMenu) {
        drawSubmenuArrow(*painter, QPointF(row.right() + 1 - kPaddingX - 2, row.center().y() + 0.5),
                         muted);
    }

    painter->restore();
}

} // namespace scribe::app
