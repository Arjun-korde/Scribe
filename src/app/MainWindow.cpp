#include "MainWindow.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLatin1String>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

namespace
{

using scribe::core::DocumentError;

// Every command id in one place, so a typo is a compile error instead of a dead menu entry.
constexpr QLatin1String kFileNew("file.new");
constexpr QLatin1String kFileOpen("file.open");
constexpr QLatin1String kFileSave("file.save");
constexpr QLatin1String kFileSaveAs("file.saveAs");
constexpr QLatin1String kFileExit("file.exit");
constexpr QLatin1String kTabClose("tab.close");
constexpr QLatin1String kTabCloseOthers("tab.closeOthers");
constexpr QLatin1String kTabCloseAll("tab.closeAll");
constexpr QLatin1String kTabNext("tab.next");
constexpr QLatin1String kTabPrevious("tab.previous");

QString fileFilter()
{
    return MainWindow::tr("Text Documents (*.txt);;All Files (*)");
}

} // namespace

FileDialogs defaultFileDialogs()
{
    FileDialogs dialogs;
    dialogs.chooseOpenPath = [](QWidget* parent) {
        return QFileDialog::getOpenFileName(parent, MainWindow::tr("Open"), QString(),
                                            fileFilter());
    };
    dialogs.chooseSavePath = [](QWidget* parent, const QString& suggestedPath) {
        return QFileDialog::getSaveFileName(parent, MainWindow::tr("Save As"), suggestedPath,
                                            fileFilter());
    };
    dialogs.showError = [](QWidget* parent, const QString& title, const QString& message) {
        QMessageBox::warning(parent, title, message);
    };
    return dialogs;
}

MainWindow::MainWindow(QWidget* parent)
    : MainWindow(defaultFileDialogs(), parent)
{}

MainWindow::MainWindow(FileDialogs dialogs, QWidget* parent)
    : QMainWindow(parent)
    , m_dialogs(std::move(dialogs))
    , m_tabs(new scribe::editor::EditorTabs(this))
{
    resize(900, 600);
    setCentralWidget(m_tabs);

    registerCommands();
    buildMenus();

    connect(m_tabs, &scribe::editor::EditorTabs::currentEditorStateChanged, this,
            &MainWindow::updateTitle);
    updateTitle();
}

scribe::editor::EditorWidget* MainWindow::editor() const
{
    return m_tabs->currentEditor();
}

int MainWindow::tabCount() const
{
    return m_tabs->count();
}

void MainWindow::registerCommands()
{
    using scribe::core::Command;

    // '&' marks the menu mnemonic (Alt+letter). Exit has no shortcut: Alt+F4 is handled by Windows.
    m_commands.registerCommand(
        Command{kFileNew, tr("&New"), QStringLiteral("Ctrl+N"), [this] { newFile(); }, true});
    m_commands.registerCommand(
        Command{kFileOpen, tr("&Open..."), QStringLiteral("Ctrl+O"), [this] { openFile(); }, true});
    m_commands.registerCommand(
        Command{kFileSave, tr("&Save"), QStringLiteral("Ctrl+S"), [this] { saveFile(); }, true});
    m_commands.registerCommand(Command{kFileSaveAs, tr("Save &As..."),
                                       QStringLiteral("Ctrl+Shift+S"), [this] { saveFileAs(); },
                                       true});
    m_commands.registerCommand(Command{kTabClose, tr("&Close Tab"), QStringLiteral("Ctrl+W"),
                                       [this] { m_tabs->closeTab(m_tabs->currentIndex()); }, true});
    m_commands.registerCommand(Command{kTabCloseOthers, tr("Close &Other Tabs"), QString(),
                                       [this] { m_tabs->closeOtherTabs(m_tabs->currentIndex()); },
                                       true});
    m_commands.registerCommand(Command{kTabCloseAll, tr("Close A&ll Tabs"),
                                       QStringLiteral("Ctrl+Shift+W"),
                                       [this] { m_tabs->closeAllTabs(); }, true});
    m_commands.registerCommand(
        Command{kFileExit, tr("E&xit"), QString(), [this] { close(); }, true});

    m_commands.registerCommand(Command{kTabNext, tr("&Next Tab"), QStringLiteral("Ctrl+Tab"),
                                       [this] { m_tabs->nextTab(); }, true});
    m_commands.registerCommand(Command{kTabPrevious, tr("&Previous Tab"),
                                       QStringLiteral("Ctrl+Shift+Tab"),
                                       [this] { m_tabs->previousTab(); }, true});
}

void MainWindow::buildMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    addCommandAction(fileMenu, kFileNew);
    addCommandAction(fileMenu, kFileOpen);
    addCommandAction(fileMenu, kFileSave);
    addCommandAction(fileMenu, kFileSaveAs);
    fileMenu->addSeparator();
    addCommandAction(fileMenu, kTabClose);
    addCommandAction(fileMenu, kTabCloseOthers);
    addCommandAction(fileMenu, kTabCloseAll);
    fileMenu->addSeparator();
    addCommandAction(fileMenu, kFileExit);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    addCommandAction(viewMenu, kTabNext);
    addCommandAction(viewMenu, kTabPrevious);
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

void MainWindow::newFile()
{
    m_tabs->addEditor();
}

void MainWindow::openFile()
{
    const QString path = m_dialogs.chooseOpenPath(this);
    if (path.isEmpty()) {
        return;
    }

    const scribe::core::IoResult result = m_tabs->openFile(path);
    if (!result.ok()) {
        showIoError(result, path);
    }
}

void MainWindow::saveFile()
{
    if (editor()->fileDocument().filePath().isEmpty()) {
        saveFileAs();
        return;
    }

    const scribe::core::IoResult result = editor()->saveFile();
    if (!result.ok()) {
        showIoError(result, editor()->fileDocument().filePath());
    }
}

void MainWindow::saveFileAs()
{
    const QString path = m_dialogs.chooseSavePath(this, editor()->fileDocument().filePath());
    if (path.isEmpty()) {
        return;
    }

    const scribe::core::IoResult result = editor()->saveFileAs(path);
    if (!result.ok()) {
        showIoError(result, path);
    }
}

void MainWindow::showIoError(const scribe::core::IoResult& result, const QString& path)
{
    const QString name = QFileInfo(path).fileName();

    QString message;
    switch (result.error) {
        case DocumentError::None:
            return;
        case DocumentError::CannotRead:
            message = tr("Could not open \"%1\".").arg(name);
            break;
        case DocumentError::InvalidEncoding:
            message =
                tr("\"%1\" was not opened because its text could not be read without changing "
                   "it.")
                    .arg(name);
            break;
        case DocumentError::CannotEncode:
            message =
                tr("\"%1\" was not saved because the text contains characters that the "
                   "%2 encoding cannot store.")
                    .arg(name, scribe::core::encodingName(editor()->fileDocument().encoding()));
            break;
        case DocumentError::CannotWrite:
            message = tr("Could not save \"%1\". The file on disk was not changed.").arg(name);
            break;
    }

    if (!result.detail.isEmpty()) {
        message += QStringLiteral("\n\n") + result.detail;
    }
    m_dialogs.showError(this, tr("Scribe"), message);
}
