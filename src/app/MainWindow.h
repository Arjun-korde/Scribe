#pragma once

#include "CommandRegistry.h"
#include "Document.h"

#include <QMainWindow>

#include <functional>

namespace scribe::editor
{
class EditorTabs;
class EditorWidget;
} // namespace scribe::editor

/// The user interaction the window needs from the outside world. The defaults show the standard
/// Qt dialogs; tests replace them with scripted answers so no dialog ever blocks.
struct FileDialogs
{
    /// Asks which file to open. An empty result means the user cancelled.
    std::function<QString(QWidget* parent)> chooseOpenPath;

    /// Asks where to save. `suggestedPath` is the current file, if any. Empty means cancelled.
    std::function<QString(QWidget* parent, const QString& suggestedPath)> chooseSavePath;

    /// Tells the user that something failed.
    std::function<void(QWidget* parent, const QString& title, const QString& message)> showError;
};

FileDialogs defaultFileDialogs();

/// The main window: a menu bar and a row of editor tabs. Every menu entry runs a command from the
/// CommandRegistry, so shortcuts, menus and later the command palette share the same actions.
/// File commands (save, save as) act on the active tab.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    explicit MainWindow(FileDialogs dialogs, QWidget* parent = nullptr);

    const scribe::core::CommandRegistry& commands() const { return m_commands; }

    /// The editor of the active tab. Never null.
    scribe::editor::EditorWidget* editor() const;
    int tabCount() const;

private:
    void registerCommands();
    void buildMenus();
    QAction* addCommandAction(QMenu* menu, const QString& commandId);
    void updateTitle();

    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void showIoError(const scribe::core::IoResult& result, const QString& path);

    FileDialogs m_dialogs;
    scribe::core::CommandRegistry m_commands;
    scribe::editor::EditorTabs* m_tabs = nullptr; // owned by Qt as the central widget
};
