#include "ThemeManager.h"

#include "MenuStyle.h"

#include <QApplication>
#include <QStyleHints>

namespace scribe::app
{

ThemeManager::ThemeManager(QApplication& app)
    : m_app(app)
{
    // Menus are painted by our style, on top of the style Windows gave us. Set before the palette,
    // because choosing a style resets the palette.
    app.setStyle(new MenuStyle(app.style()->name()));

    const QStyleHints* hints = app.styleHints();
    connect(hints, &QStyleHints::colorSchemeChanged, this, &ThemeManager::follow);
    follow(hints->colorScheme());
}

Theme ThemeManager::themeFor(Qt::ColorScheme scheme)
{
    return scheme == Qt::ColorScheme::Dark ? Theme::dark() : Theme::light();
}

void ThemeManager::follow(Qt::ColorScheme scheme)
{
    m_theme = themeFor(scheme);
    m_app.setPalette(m_theme.palette());
    m_app.setStyleSheet(m_theme.styleSheet());
    emit themeChanged(m_theme);
}

} // namespace scribe::app
