#include "MainWindow.h"

#include "DocumentController.h"
#include "EditCommands.h"
#include "EditorTabs.h"
#include "EditorWidget.h"
#include "FileCommands.h"
#include "TabCommands.h"

#include <QAction>
#include <QCloseEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>

using scribe::editor::EditCommands;
using scribe::editor::TabCommands;

MainWindow::MainWindow(QWidget* parent)
    : MainWindow(defaultFileDialogs(), parent)
{}

MainWindow::MainWindow(FileDialogs dialogs, QWidget* parent)
    : QMainWindow(parent)
    , m_tabs(new scribe::editor::EditorTabs(this))
    , m_controller(std::make_unique<DocumentController>(*m_tabs, std::move(dialogs), this))
{
    resize(900, 600);
    setCentralWidget(m_tabs);

    registerCommands();
    buildMenus();

    connect(m_tabs, &scribe::editor::EditorTabs::currentEditorStateChanged, this,
            &MainWindow::updateTitle);
    connect(m_tabs, &scribe::editor::EditorTabs::filesDropped, this,
            [this](const QStringList& paths) { m_controller->openFiles(paths); });
    updateTitle();
}

MainWindow::~MainWindow() = default;

scribe::editor::EditorWidget* MainWindow::editor() const
{
    return m_tabs->currentEditor();
}

int MainWindow::tabCount() const
{
    return m_tabs->count();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Asks about every modified tab; one Cancel keeps the window open.
    if (m_tabs->closeAllTabs()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::registerCommands()
{
    FileCommands::registerIn(m_commands, *m_controller, *this);
    EditCommands::registerIn(m_commands, *m_tabs);
    TabCommands::registerIn(m_commands, *m_tabs);
}

void MainWindow::buildMenus()
{
    // Menus show a command as available only if it can run right now (for example Copy needs a
    // selection), so they ask the registry each time they open.
    const auto addMenu = [this](const QString& title) {
        QMenu* menu = menuBar()->addMenu(title);
        connect(menu, &QMenu::aboutToShow, this, [this, menu] { refreshMenuState(menu); });
        return menu;
    };

    QMenu* fileMenu = addMenu(tr("&File"));
    addCommandAction(fileMenu, FileCommands::kNew);
    addCommandAction(fileMenu, FileCommands::kOpen);
    addCommandAction(fileMenu, FileCommands::kSave);
    addCommandAction(fileMenu, FileCommands::kSaveAs);
    fileMenu->addSeparator();
    addCommandAction(fileMenu, TabCommands::kClose);
    addCommandAction(fileMenu, TabCommands::kCloseOthers);
    addCommandAction(fileMenu, TabCommands::kCloseAll);
    fileMenu->addSeparator();
    addCommandAction(fileMenu, FileCommands::kExit);

    QMenu* editMenu = addMenu(tr("&Edit"));
    addCommandAction(editMenu, EditCommands::kUndo);
    addCommandAction(editMenu, EditCommands::kRedo);
    editMenu->addSeparator();
    addCommandAction(editMenu, EditCommands::kCut);
    addCommandAction(editMenu, EditCommands::kCopy);
    addCommandAction(editMenu, EditCommands::kPaste);
    addCommandAction(editMenu, EditCommands::kDelete);
    editMenu->addSeparator();
    addCommandAction(editMenu, EditCommands::kSelectAll);
    addCommandAction(editMenu, EditCommands::kInsertDateTime);

    QMenu* viewMenu = addMenu(tr("&View"));
    addCommandAction(viewMenu, TabCommands::kNext);
    addCommandAction(viewMenu, TabCommands::kPrevious);
}

void MainWindow::refreshMenuState(QMenu* menu)
{
    for (QAction* action : menu->actions()) {
        if (!action->isSeparator()) {
            action->setEnabled(m_commands.isEnabled(action->objectName()));
        }
    }
}

QAction* MainWindow::addCommandAction(QMenu* menu, const QString& commandId)
{
    const scribe::core::Command* command = m_commands.find(commandId);
    Q_ASSERT(command != nullptr);

    auto* action = menu->addAction(command->title);
    action->setObjectName(command->id);
    if (!command->shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(command->shortcut, QKeySequence::PortableText));
    }
    connect(action, &QAction::triggered, this, [this, commandId] { m_commands.run(commandId); });
    return action;
}

void MainWindow::updateTitle()
{
    // "[*]" is replaced by Qt with a marker while the window is modified. A file name that
    // contains the placeholder itself must be escaped.
    QString name = editor()->displayName();
    name.replace(QStringLiteral("[*]"), QStringLiteral("[[*]]"));

    setWindowTitle(QStringLiteral("%1[*] - %2").arg(name, tr("Scribe")));
    setWindowModified(editor()->isModified());
}
