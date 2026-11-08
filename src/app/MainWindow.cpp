#include "MainWindow.h"

#include "DocumentController.h"
#include "EditCommands.h"
#include "EditorStatusBar.h"
#include "EditorTabs.h"
#include "EditorWidget.h"
#include "EncodingCommands.h"
#include "FileCommands.h"
#include "FindBar.h"
#include "SearchCommands.h"
#include "TabCommands.h"
#include "ViewCommands.h"

#include <QAction>
#include <QCloseEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>

using scribe::editor::EditCommands;
using scribe::editor::SearchCommands;
using scribe::editor::TabCommands;
using scribe::editor::ViewCommands;

MainWindow::MainWindow(QWidget* parent)
    : MainWindow(defaultFileDialogs(), parent)
{}

MainWindow::MainWindow(FileDialogs dialogs, QWidget* parent)
    : QMainWindow(parent)
    , m_tabs(new scribe::editor::EditorTabs(this))
    , m_dialogs(dialogs) // copied before the controller takes the dialogs
    , m_controller(std::make_unique<DocumentController>(*m_tabs, std::move(dialogs), this))
{
    resize(900, 600);
    // The tabs with the find bar under them.
    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    m_findBar = new scribe::editor::FindBar(*m_tabs, central);
    centralLayout->addWidget(m_tabs, 1);
    centralLayout->addWidget(m_findBar);
    setCentralWidget(central);
    m_statusBar = new scribe::editor::EditorStatusBar(*m_tabs, this);
    setStatusBar(m_statusBar);

    registerCommands();
    buildMenus();

    connect(m_tabs, &scribe::editor::EditorTabs::currentEditorStateChanged, this,
            &MainWindow::updateTitle);
    connect(m_tabs, &scribe::editor::EditorTabs::filesDropped, this,
            [this](const QStringList& paths) { m_controller->openFiles(paths); });
    connect(m_statusBar, &scribe::editor::EditorStatusBar::positionClicked, this,
            [this] { m_commands.run(SearchCommands::kGoToLine); });
    connect(m_statusBar, &scribe::editor::EditorStatusBar::encodingClicked, this,
            [this](QWidget* anchor) { showChoices(anchor, m_encodingActions); });
    connect(m_statusBar, &scribe::editor::EditorStatusBar::lineEndingClicked, this,
            [this](QWidget* anchor) { showChoices(anchor, m_lineEndingActions); });
    updateTitle();

    // The new window must start with the caret in the text, so that typing works at once. (The find
    // bar sits in the same layout, and without this the first focus can land elsewhere.)
    editor()->setFocus();
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
    EncodingCommands::registerIn(m_commands, *m_controller, *m_tabs);

    // A chooser is left empty when its dialog is missing, which makes the command unavailable.
    ViewCommands::FontChooser fontChooser;
    if (m_dialogs.chooseFont) {
        fontChooser = [this](const QFont& current) { return m_dialogs.chooseFont(this, current); };
    }
    ViewCommands::registerIn(m_commands, *m_tabs, *m_statusBar, fontChooser);

    SearchCommands::LineNumberChooser lineChooser;
    if (m_dialogs.askLineNumber) {
        lineChooser = [this](int currentLine, int lineCount) {
            return m_dialogs.askLineNumber(this, currentLine, lineCount);
        };
    }
    SearchCommands::registerIn(m_commands, *m_tabs, *m_findBar, lineChooser);

    TabCommands::registerIn(m_commands, *m_tabs);
}

void MainWindow::buildMenus()
{
    const auto addMenu = [this](const QString& title) {
        QMenu* menu = menuBar()->addMenu(title);
        watchMenu(menu);
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

    QMenu* searchMenu = addMenu(tr("&Search"));
    addCommandAction(searchMenu, SearchCommands::kFind);
    addCommandAction(searchMenu, SearchCommands::kFindNext);
    addCommandAction(searchMenu, SearchCommands::kFindPrevious);
    addCommandAction(searchMenu, SearchCommands::kReplace);
    searchMenu->addSeparator();
    addCommandAction(searchMenu, SearchCommands::kGoToLine);

    QMenu* viewMenu = addMenu(tr("&View"));
    addCommandAction(viewMenu, ViewCommands::kWordWrap);
    addCommandAction(viewMenu, ViewCommands::kFont);
    viewMenu->addSeparator();
    addCommandAction(viewMenu, ViewCommands::kZoomIn);
    addCommandAction(viewMenu, ViewCommands::kZoomOut);
    addCommandAction(viewMenu, ViewCommands::kZoomReset);
    viewMenu->addSeparator();
    addCommandAction(viewMenu, ViewCommands::kStatusBar);
    viewMenu->addSeparator();
    addCommandAction(viewMenu, TabCommands::kNext);
    addCommandAction(viewMenu, TabCommands::kPrevious);

    // How the active document is written on the next save. The same actions are offered from the
    // status bar, so they are remembered in two lists.
    QMenu* encodingMenu = addMenu(tr("En&coding"));
    for (const auto& entry : EncodingCommands::encodings()) {
        m_encodingActions.append(addCommandAction(encodingMenu, entry.id));
    }
    encodingMenu->addSeparator();
    for (const auto& entry : EncodingCommands::lineEndings()) {
        m_lineEndingActions.append(addCommandAction(encodingMenu, entry.id));
    }
}

void MainWindow::watchMenu(QMenu* menu)
{
    // Menus show a command as available only if it can run right now (for example Copy needs a
    // selection), so they ask the registry each time they open. When a menu closes every entry
    // is switched back on: a disabled QAction also disables its shortcut, and the state computed
    // at opening time is stale a moment later. The registry refuses commands that cannot run.
    connect(menu, &QMenu::aboutToShow, this, [this, menu] { refreshMenuState(menu); });
    connect(menu, &QMenu::aboutToHide, this, [menu] {
        for (QAction* action : menu->actions()) {
            action->setEnabled(true);
        }
    });
}

void MainWindow::showChoices(QWidget* anchor, const QList<QAction*>& actions)
{
    // A pop-up built from the actions that already exist, so its check marks and behavior are
    // exactly those of the Encoding menu. It deletes itself when it closes.
    auto* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addActions(actions);
    watchMenu(menu);

    // The status bar is at the bottom of the window, so open the menu upwards from the segment.
    QPoint position = anchor->mapToGlobal(QPoint(0, 0));
    position.setY(position.y() - menu->sizeHint().height());
    menu->popup(position);
}

void MainWindow::refreshMenuState(QMenu* menu)
{
    for (QAction* action : menu->actions()) {
        if (!action->isSeparator()) {
            action->setEnabled(m_commands.isEnabled(action->objectName()));
            if (action->isCheckable()) {
                action->setChecked(m_commands.isChecked(action->objectName()));
            }
        }
    }
}

QAction* MainWindow::addCommandAction(QMenu* menu, const QString& commandId)
{
    const scribe::core::Command* command = m_commands.find(commandId);
    Q_ASSERT(command != nullptr);

    auto* action = menu->addAction(command->title);
    action->setObjectName(command->id);

    QList<QKeySequence> shortcuts;
    for (const QString& text : {command->shortcut, command->alternateShortcut}) {
        if (!text.isEmpty()) {
            shortcuts.append(QKeySequence(text, QKeySequence::PortableText));
        }
    }
    action->setShortcuts(shortcuts); // the first one is shown in the menu

    // A toggle (word wrap, status bar) is shown with a check mark that follows the command.
    action->setCheckable(static_cast<bool>(command->checkedWhen));
    action->setChecked(m_commands.isChecked(commandId));

    connect(action, &QAction::triggered, this, [this, commandId, action] {
        m_commands.run(commandId);
        // Qt flips a checkable action by itself; bring it back in line with the real state.
        if (action->isCheckable()) {
            action->setChecked(m_commands.isChecked(commandId));
        }
    });
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
