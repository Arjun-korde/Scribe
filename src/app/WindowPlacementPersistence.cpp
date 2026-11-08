#include "WindowPlacementPersistence.h"

#include "SettingsManager.h"

#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QStyle>
#include <QWidget>

namespace scribe::app
{

WindowPlacementPersistence::WindowPlacementPersistence(QWidget& window,
                                                       core::SettingsManager& settings,
                                                       int debounceMs, int armDelayMs,
                                                       QObject* parent)
    : QObject(parent)
    , m_window(&window)
    , m_settings(settings)
    , m_armDelayMs(armDelayMs)
{
    // Restore before listening, so that restoring is not mistaken for the user moving the window.
    if (const auto saved = core::loadWindowPlacement(settings)) {
        QList<QRect> screens;
        for (const QScreen* screen : QGuiApplication::screens()) {
            screens.append(screen->availableGeometry());
        }
        const int titleBar = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
        if (const auto fitted = core::fitToScreens(saved->rect, screens, titleBar)) {
            window.setGeometry(*fitted);
            if (saved->maximized) {
                window.setWindowState(window.windowState() | Qt::WindowMaximized);
            }
        }
    }

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(debounceMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &WindowPlacementPersistence::saveIfChanged);

    m_armTimer.setSingleShot(true);
    m_armTimer.setInterval(qMax(armDelayMs, 0));
    connect(&m_armTimer, &QTimer::timeout, this, &WindowPlacementPersistence::arm);

    window.installEventFilter(this);
}

WindowPlacementPersistence::~WindowPlacementPersistence()
{
    saveIfChanged();
}

void WindowPlacementPersistence::arm()
{
    m_armTimer.stop();
    m_armed = true;
    capture();
    m_saved = m_pending;
}

void WindowPlacementPersistence::flush()
{
    m_saveTimer.stop();
    capture();
    saveIfChanged();
}

bool WindowPlacementPersistence::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window) {
        switch (event->type()) {
            case QEvent::Show:
                if (!m_armed && m_armDelayMs >= 0 && !m_armTimer.isActive()) {
                    m_armTimer.start();
                }
                break;
            case QEvent::Move:
            case QEvent::Resize:
            case QEvent::WindowStateChange:
                if (m_armed) {
                    capture();
                    m_saveTimer.start();
                }
                break;
            default:
                break;
        }
    }
    return false;
}

void WindowPlacementPersistence::capture()
{
    if (!m_armed || m_window->isFullScreen()) {
        return;
    }
    // The normal (not maximized) rectangle, so un-maximizing later goes back to the right place.
    m_pending = core::WindowPlacement{.rect = m_window->normalGeometry(),
                                      .maximized = m_window->isMaximized()};
}

void WindowPlacementPersistence::saveIfChanged()
{
    if (m_armed && m_pending && m_pending != m_saved) {
        core::saveWindowPlacement(m_settings, *m_pending);
        m_saved = m_pending;
    }
}

} // namespace scribe::app
