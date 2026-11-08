#pragma once

#include "CommandRegistry.h"
#include "FileDialogs.h"

#include <QMainWindow>

#include <memory>

class DocumentController;

namespace scribe::core
{
class SettingsManager;
}

namespace scribe::editor
{
class EditorStatusBar;
class FindBar;
class EditorTabs;
class EditorWidget;
class ViewSettingsPersistence;
} // namespace scribe::editor

/// The main window: a menu bar and a row of editor tabs. Every menu entry runs a command from the
/// CommandRegistry, so shortcuts, menus and later the command palette share the same actions.
/// Closing the window asks about unsaved changes in every modified tab.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    explicit MainWindow(FileDialogs dialogs, QWidget* parent = nullptr);

    /// With a settings store the window starts with the saved view settings (word wrap, font,
    /// zoom) and saves changes to them. `settings` may be null, which saves nothing, and must
    /// outlive the window.
    MainWindow(FileDialogs dialogs, scribe::core::SettingsManager* settings,
               QWidget* parent = nullptr);
    ~MainWindow() override;

    const scribe::core::CommandRegistry& commands() const { return m_commands; }

    /// The editor of the active tab. Never null.
    scribe::editor::EditorWidget* editor() const;
    int tabCount() const;

    /// The find and replace bar under the tabs (hidden until opened).
    scribe::editor::FindBar* findBar() const { return m_findBar; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void registerCommands();
    void buildMenus();
    void refreshMenuState(QMenu* menu);
    void watchMenu(QMenu* menu);
    void showChoices(QWidget* anchor, const QList<QAction*>& actions);
    QAction* addCommandAction(QMenu* menu, const QString& commandId);
    void updateTitle();

    scribe::core::CommandRegistry m_commands;
    scribe::editor::EditorTabs* m_tabs = nullptr;           // owned by Qt as the central widget
    scribe::editor::EditorStatusBar* m_statusBar = nullptr; // the window's status bar, owned by Qt
    scribe::editor::FindBar* m_findBar = nullptr;           // under the tabs, owned by Qt
    FileDialogs m_dialogs; // a copy, for the commands that ask the user (font, line number)
    // The Encoding menu's actions, also offered from the status bar's pop-ups.
    QList<QAction*> m_encodingActions;
    QList<QAction*> m_lineEndingActions;
    // Declared last, so it is destroyed first: it holds a guard installed on m_tabs.
    std::unique_ptr<DocumentController> m_controller;
    // Loads and saves the view settings; it only needs the settings store, not the widgets.
    std::unique_ptr<scribe::editor::ViewSettingsPersistence> m_viewPersistence;
};
