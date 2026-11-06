#pragma once

#include "CommandRegistry.h"
#include "FileDialogs.h"

#include <QMainWindow>

#include <memory>

class DocumentController;

namespace scribe::editor
{
class EditorStatusBar;
class EditorTabs;
class EditorWidget;
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
    ~MainWindow() override;

    const scribe::core::CommandRegistry& commands() const { return m_commands; }

    /// The editor of the active tab. Never null.
    scribe::editor::EditorWidget* editor() const;
    int tabCount() const;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void registerCommands();
    void buildMenus();
    void refreshMenuState(QMenu* menu);
    QAction* addCommandAction(QMenu* menu, const QString& commandId);
    void updateTitle();

    scribe::core::CommandRegistry m_commands;
    scribe::editor::EditorTabs* m_tabs = nullptr;           // owned by Qt as the central widget
    scribe::editor::EditorStatusBar* m_statusBar = nullptr; // the window's status bar, owned by Qt
    decltype(FileDialogs::chooseFont) m_chooseFont;         // for the View > Font command
    // Declared last, so it is destroyed first: it holds a guard installed on m_tabs.
    std::unique_ptr<DocumentController> m_controller;
};
