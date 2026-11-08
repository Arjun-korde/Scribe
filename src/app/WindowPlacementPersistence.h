#pragma once

#include "WindowPlacement.h"

#include <QObject>
#include <QTimer>

#include <optional>

class QWidget;

namespace scribe::core
{
class SettingsManager;
}

namespace scribe::app
{

/// Remembers where the main window is between runs: its size, position and whether it is
/// maximized (see core/WindowPlacement.h for the file format).
///
/// When created it moves the window to the saved place, first checking that the place is still on
/// a connected screen (a monitor may have been unplugged) and shrinking or moving the window into
/// view if needed. A saved place that is on no screen is ignored and the window keeps its size.
///
/// Afterwards moving, resizing and maximizing are saved shortly after they stop. Placement is only
/// saved once the window has settled after its first display, and only if it then differs from
/// where it started, so merely opening and closing the program never creates a settings file.
///
/// It keeps its own copy of the latest placement, so the destructor can save without touching the
/// window. The settings store must outlive this object.
class WindowPlacementPersistence : public QObject
{
    Q_OBJECT

public:
    /// `armDelayMs` is how long after the window is first shown the starting place is noted down
    /// (the system may still be adjusting it); a negative value waits for arm() instead.
    WindowPlacementPersistence(QWidget& window, core::SettingsManager& settings,
                               int debounceMs = 400, int armDelayMs = 500,
                               QObject* parent = nullptr);
    ~WindowPlacementPersistence() override;

    /// Notes the window's current place as the starting point and starts watching for changes.
    void arm();

    /// Saves the current placement now if it changed since the start or the last save.
    void flush();

    [[nodiscard]] bool isArmed() const { return m_armed; }

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void capture();
    void saveIfChanged();

    QWidget* m_window;
    core::SettingsManager& m_settings;
    QTimer m_saveTimer;
    QTimer m_armTimer;
    int m_armDelayMs;
    bool m_armed = false;
    std::optional<core::WindowPlacement> m_pending; // the latest placement seen
    std::optional<core::WindowPlacement> m_saved;   // the starting point or the last one saved
};

} // namespace scribe::app
