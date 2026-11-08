#include "ThemeManager.h"

#include <QApplication>
#include <QStyleHints>

namespace scribe::app
{

ThemeManager::ThemeManager(QApplication& app)
    : m_app(app)
{
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
