#pragma once

#include "Theme.h"

#include <QObject>

class QApplication;

namespace scribe::app
{

/// Applies a `Theme` to the whole application and switches it when Windows changes between its
/// light and dark setting. A manual override comes with the Settings dialog.
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    /// Applies the theme matching the current Windows setting straight away.
    explicit ThemeManager(QApplication& app);

    const Theme& theme() const { return m_theme; }

    /// The theme for a colour scheme. `Unknown` (no OS preference) gives the light theme.
    static Theme themeFor(Qt::ColorScheme scheme);

signals:
    void themeChanged(const scribe::app::Theme& theme);

private:
    void follow(Qt::ColorScheme scheme);

    QApplication& m_app;
    Theme m_theme;
};

} // namespace scribe::app
