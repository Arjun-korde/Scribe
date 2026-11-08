#include "MainWindow.h"

#include "EditorStatusBar.h"
#include "EditorWidget.h"
#include "FindBar.h"
#include "SettingsManager.h"

#include <QAction>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QTemporaryDir>
#include <QToolButton>
#include <QUrl>
#include <QWheelEvent>
#include <QtTest>

#include <memory>
#include <optional>

using scribe::editor::EditorWidget;

namespace
{

// Menus are found by their title, so adding a menu does not renumber the others.
QMenu* menuTitled(const MainWindow& window, const QString& title)
{
    for (QMenu* menu : window.findChildren<QMenu*>()) {
        if (menu->title() == title) {
            return menu;
        }
    }
    return nullptr;
}

} // namespace

class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void startsUntitledAndClean();
    void fileMenuIsBuiltFromRegisteredCommands();
    void openLoadsTheChosenFile();
    void cancelledOpenChangesNothing();
    void failedOpenShowsAnErrorAndKeepsContent();
    void saveOfUntitledAsksForAPath();
    void saveOfNamedFileDoesNotAsk();
    void cancelledSaveAsWritesNothing();
    void failedSaveShowsAnErrorAndStaysModified();
    void unencodableTextExplainsTheEncoding();
    void titleShowsModifiedMarker();
    void newOpensAnotherTabAndKeepsTheFirst();
    void openGoesToANewTabWhenTheCurrentOneIsInUse();
    void titleFollowsTheActiveTab();
    void saveActsOnTheActiveTab();
    void closeCommandsManageTabs();
    void closingTheLastTabKeepsTheWindowUsable();
    void realKeystrokesSwitchTabsWhileTheEditorHasFocus();

    void closingTheWindowAsksAboutUnsavedChanges();
    void cancelKeepsTheWindowOpen();
    void discardClosesTheWindowWithoutSaving();
    void saveWritesTheFileThenClosesTheWindow();
    void exitCommandAsksToo();
    void cleanWindowClosesWithoutAsking();
    void cancelOnALaterTabKeepsTheWindowAndEarlierClosures();
    void newAndOpenDoNotAsk();
    void closeTabCommandAsksAboutThatTab();
    void exitClosesTheWindow();

    void editMenuEntriesAreAvailableOnlyWhenTheyCanRun();
    void editMenuEntriesRunTheirCommands();
    void realKeystrokesEditTheText();

    void droppingFilesOnTheWindowOpensThemInTabs();
    void droppingAFailingFileShowsAnError();

    void viewMenuHasTheViewCommandsWithShortcuts();
    void viewMenuChecksFollowWordWrapAndStatusBar();
    void wordWrapMenuEntryTogglesEveryTab();
    void statusBarMenuEntryHidesAndShowsTheBar();
    void statusBarFollowsTypingAndTabsInTheWindow();
    void fontMenuEntryAppliesTheChosenFont();
    void fontMenuEntryIsUnavailableWithoutADialog();
    void realZoomKeystrokesChangeTheZoomByTenPercent();
    void zoomResetShortcutWorksAfterTheMenuHasBeenOpenedAtOneHundred();
    void ctrlWheelZoomsTheWindow();

    void encodingMenuHasEveryEncodingAndLineEnding();
    void encodingMenuChecksFollowTheActiveDocument();
    void choosingFromTheEncodingMenuUpdatesTheStatusBarAndTab();
    void savingAfterChoosingFromTheMenuWritesTheNewFormat();
    void refusedAnsiShowsAnErrorInTheWindow();
    void clickingTheEncodingSegmentOpensTheEncodingChoices();
    void clickingTheLineEndingSegmentOpensTheLineEndingChoices();
    void choosingFromTheStatusBarPopupChangesTheDocument();
    void statusBarPopupsReuseTheMenuActions();

    void searchMenuHasTheFindAndGoToLineCommands();
    void goToLineAsksWithTheCurrentLineAndLineCountThenMovesTheCaret();
    void cancellingGoToLineKeepsTheCaret();
    void goToLineUpdatesTheStatusBar();
    void clickingTheLineColumnSegmentOpensGoToLine();
    void realCtrlGOpensGoToLine();
    void goToLineIsUnavailableWithoutADialog();
    void goToLineActsOnTheActiveTab();

    void findBarIsHiddenAtStartupAndEditorHasTheFocus();
    void searchMenuHasTheShortcuts();
    void searchMenuEntriesOpenTheBar();
    void ctrlFOpensTheBarWithTheFocusInTheFindField();
    void typingInTheBarSelectsMatchesInTheEditor();
    void f3AndShiftF3NavigateWithTheBarClosed();
    void ctrlHOpensTheReplaceRow();
    void escapeReturnsTheFocusToTheEditor();
    void replaceAllThenCtrlZRestoresTheTextInOneStep();
    void replaceAllMarksTheWindowModified();
    void ctrlFTakesTheSelectedTextAsTheQuery();
    void shortcutsDoNotTypeIntoTheText();

    void aWindowWithASettingsStoreStartsWithTheSavedViewSettings();
    void viewChangesAreSavedWhenTheWindowCloses();
    void theStatusBarAndFindOptionsAreRemembered();
    void aSecondWindowStartsWhereTheFirstLeftOff();
    void theFontChosenInTheDialogIsRemembered();
    void zoomFromTheMouseWheelIsRemembered();
    void theStatusBarShowsTheSavedZoomFromTheStart();
    void aWindowWithoutAStoreStillWorksAndSavesNothing();
    void aBrokenSettingsFileStillGivesAWorkingWindow();

private:
    QString path(const QString& name) const { return m_dir->filePath(name); }
    void write(const QString& name, const QByteArray& bytes) const;
    QByteArray read(const QString& name) const;
    void type(MainWindow& window, const QString& text) const;
    static void dragAndDrop(QWidget* target, const QMimeData& mime);

    // Scripted answers for the dialogs, plus a record of what was asked.
    FileDialogs scriptedDialogs();
    QString m_openAnswer;
    QString m_saveAnswer;
    int m_openAsked = 0;
    int m_saveAsked = 0;
    QStringList m_errors;
    QList<UnsavedChoice> m_choices; // consumed in order; Cancel once they run out
    QStringList m_promptedFor;
    std::optional<QFont> m_fontAnswer; // what the scripted font dialog returns
    QList<QFont> m_fontDialogOpenedWith;
    std::optional<int> m_lineAnswer;              // what the scripted Go To Line dialog returns
    QList<QPair<int, int>> m_lineDialogAskedWith; // (current line, line count) per call

    std::unique_ptr<QTemporaryDir> m_dir;
};

void MainWindowTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_openAnswer.clear();
    m_saveAnswer.clear();
    m_openAsked = 0;
    m_saveAsked = 0;
    m_errors.clear();
    m_choices.clear();
    m_promptedFor.clear();
    m_fontAnswer.reset();
    m_fontDialogOpenedWith.clear();
    m_lineAnswer.reset();
    m_lineDialogAskedWith.clear();
}

FileDialogs MainWindowTest::scriptedDialogs()
{
    FileDialogs dialogs;
    dialogs.chooseOpenPath = [this](QWidget*) {
        ++m_openAsked;
        return m_openAnswer;
    };
    dialogs.chooseSavePath = [this](QWidget*, const QString&) {
        ++m_saveAsked;
        return m_saveAnswer;
    };
    dialogs.showError = [this](QWidget*, const QString&, const QString& message) {
        m_errors.append(message);
    };
    dialogs.confirmDiscard = [this](QWidget*, const QString& name) {
        m_promptedFor.append(name);
        return m_choices.isEmpty() ? UnsavedChoice::Cancel : m_choices.takeFirst();
    };
    dialogs.chooseFont = [this](QWidget*, const QFont& current) {
        m_fontDialogOpenedWith.append(current);
        return m_fontAnswer;
    };
    dialogs.askLineNumber = [this](QWidget*, int currentLine, int lineCount) {
        m_lineDialogAskedWith.append({currentLine, lineCount});
        return m_lineAnswer;
    };
    return dialogs;
}

void MainWindowTest::write(const QString& name, const QByteArray& bytes) const
{
    QFile file(path(name));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(bytes);
}

QByteArray MainWindowTest::read(const QString& name) const
{
    QFile file(path(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void MainWindowTest::type(MainWindow& window, const QString& text) const
{
    window.editor()->moveCursor(QTextCursor::End);
    window.editor()->insertPlainText(text);
}

void MainWindowTest::startsUntitledAndClean()
{
    MainWindow window(scriptedDialogs());
    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - Scribe"));
    QVERIFY(!window.isWindowModified());
}

void MainWindowTest::fileMenuIsBuiltFromRegisteredCommands()
{
    MainWindow window(scriptedDialogs());

    const QList<QAction*> menus = window.menuBar()->actions();
    QCOMPARE(menus.size(), 5);
    QCOMPARE(menus.at(0)->text(), QStringLiteral("&File"));
    QCOMPARE(menus.at(1)->text(), QStringLiteral("&Edit"));
    QCOMPARE(menus.at(2)->text(), QStringLiteral("&Search"));
    QCOMPARE(menus.at(3)->text(), QStringLiteral("&View"));
    QCOMPARE(menus.at(4)->text(), QStringLiteral("En&coding"));

    // Menu entries must be the registry's commands, with the registry's shortcuts.
    const QStringList ids = {
        "file.new",    "file.open",       "file.save",          "file.saveAs", "file.exit",
        "tab.close",   "tab.closeOthers", "tab.closeAll",       "tab.next",    "tab.previous",
        "edit.undo",   "edit.redo",       "edit.cut",           "edit.copy",   "edit.paste",
        "edit.delete", "edit.selectAll",  "edit.insertDateTime"};
    for (const QString& id : ids) {
        const auto* command = window.commands().find(id);
        QVERIFY2(command != nullptr, qPrintable(id));

        auto* action = window.findChild<QAction*>(id);
        QVERIFY2(action != nullptr, qPrintable(id));
        QCOMPARE(action->text(), command->title);
        QCOMPARE(action->shortcut(), QKeySequence(command->shortcut, QKeySequence::PortableText));
    }
    QCOMPARE(window.findChild<QAction*>("file.save")->shortcut(), QKeySequence("Ctrl+S"));
    QCOMPARE(window.findChild<QAction*>("file.saveAs")->shortcut(), QKeySequence("Ctrl+Shift+S"));
    QCOMPARE(window.findChild<QAction*>("tab.close")->shortcut(), QKeySequence("Ctrl+W"));
    QCOMPARE(window.findChild<QAction*>("tab.next")->shortcut(), QKeySequence("Ctrl+Tab"));
    QCOMPARE(window.findChild<QAction*>("tab.previous")->shortcut(),
             QKeySequence("Ctrl+Shift+Tab"));
}

void MainWindowTest::openLoadsTheChosenFile()
{
    write("notes.txt", "hello\r\nworld");
    m_openAnswer = path("notes.txt");

    MainWindow window(scriptedDialogs());
    QVERIFY(window.commands().run("file.open"));

    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("hello\nworld"));
    QCOMPARE(window.windowTitle(), QStringLiteral("notes.txt[*] - Scribe"));
    QVERIFY(!window.isWindowModified());
    QCOMPARE(m_openAsked, 1);
    QVERIFY(m_errors.isEmpty());
}

void MainWindowTest::cancelledOpenChangesNothing()
{
    MainWindow window(scriptedDialogs());
    type(window, "typed");

    m_openAnswer.clear(); // the user pressed Cancel
    window.commands().run("file.open");

    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("typed"));
    QVERIFY(m_errors.isEmpty());
}

void MainWindowTest::failedOpenShowsAnErrorAndKeepsContent()
{
    MainWindow window(scriptedDialogs());
    type(window, "typed");

    m_openAnswer = path("missing.txt");
    window.commands().run("file.open");

    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("typed"));
    QCOMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("missing.txt"), qPrintable(m_errors.first()));
}

void MainWindowTest::saveOfUntitledAsksForAPath()
{
    MainWindow window(scriptedDialogs());
    type(window, "fresh");

    m_saveAnswer = path("out.txt");
    window.commands().run("file.save");

    QCOMPARE(m_saveAsked, 1);
    QCOMPARE(read("out.txt"), QByteArray("fresh"));
    QCOMPARE(window.windowTitle(), QStringLiteral("out.txt[*] - Scribe"));
    QVERIFY(!window.isWindowModified());
}

void MainWindowTest::saveOfNamedFileDoesNotAsk()
{
    write("a.txt", "abc");
    m_openAnswer = path("a.txt");
    MainWindow window(scriptedDialogs());
    window.commands().run("file.open");
    type(window, "def");

    window.commands().run("file.save");

    QCOMPARE(m_saveAsked, 0);
    QCOMPARE(read("a.txt"), QByteArray("abcdef"));
}

void MainWindowTest::cancelledSaveAsWritesNothing()
{
    MainWindow window(scriptedDialogs());
    type(window, "text");

    m_saveAnswer.clear();
    window.commands().run("file.saveAs");

    QCOMPARE(m_saveAsked, 1);
    QVERIFY(window.isWindowModified());
    QVERIFY(window.editor()->fileDocument().filePath().isEmpty());
    QVERIFY(m_errors.isEmpty());
}

void MainWindowTest::failedSaveShowsAnErrorAndStaysModified()
{
    MainWindow window(scriptedDialogs());
    type(window, "text");

    m_saveAnswer = path("no-such-folder/out.txt");
    window.commands().run("file.saveAs");

    QCOMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("out.txt"), qPrintable(m_errors.first()));
    QVERIFY(window.isWindowModified());
}

void MainWindowTest::unencodableTextExplainsTheEncoding()
{
    MainWindow window(scriptedDialogs());
    window.editor()->setEncoding({scribe::core::Encoding::Ansi, false});
    type(window, QStringLiteral("smile \U0001F600"));

    m_saveAnswer = path("out.txt");
    window.commands().run("file.saveAs");

    QCOMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("ANSI"), qPrintable(m_errors.first()));
    QVERIFY(!QFile::exists(path("out.txt")));
}

void MainWindowTest::titleShowsModifiedMarker()
{
    MainWindow window(scriptedDialogs());
    QVERIFY(!window.isWindowModified());

    type(window, "x");
    QVERIFY(window.isWindowModified());

    m_saveAnswer = path("m.txt");
    window.commands().run("file.save");
    QVERIFY(!window.isWindowModified());
}

void MainWindowTest::newOpensAnotherTabAndKeepsTheFirst()
{
    write("a.txt", "abc");
    m_openAnswer = path("a.txt");
    MainWindow window(scriptedDialogs());
    window.commands().run("file.open");
    type(window, "x");
    QCOMPARE(window.tabCount(), 1);

    window.commands().run("file.new");

    QCOMPARE(window.tabCount(), 2);
    QVERIFY(window.editor()->toPlainText().isEmpty());
    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - Scribe"));
    QVERIFY(!window.isWindowModified());

    window.commands().run("tab.previous"); // the first tab still has its unsaved edit
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("abcx"));
    QVERIFY(window.isWindowModified());
}

void MainWindowTest::openGoesToANewTabWhenTheCurrentOneIsInUse()
{
    write("a.txt", "one");
    write("b.txt", "two");
    MainWindow window(scriptedDialogs());

    m_openAnswer = path("a.txt");
    window.commands().run("file.open");
    QCOMPARE(window.tabCount(), 1); // reused the untouched Untitled tab

    m_openAnswer = path("b.txt");
    window.commands().run("file.open");
    QCOMPARE(window.tabCount(), 2);
    QCOMPARE(window.windowTitle(), QStringLiteral("b.txt[*] - Scribe"));
}

void MainWindowTest::titleFollowsTheActiveTab()
{
    MainWindow window(scriptedDialogs());
    window.commands().run("file.new");
    type(window, "edit"); // tab 2 is modified

    QVERIFY(window.isWindowModified());
    window.commands().run("tab.next"); // wraps to tab 1, which is clean
    QVERIFY(!window.isWindowModified());
    window.commands().run("tab.previous");
    QVERIFY(window.isWindowModified());
}

void MainWindowTest::saveActsOnTheActiveTab()
{
    MainWindow window(scriptedDialogs());
    type(window, "first");
    window.commands().run("file.new");
    type(window, "second");

    m_saveAnswer = path("second.txt");
    window.commands().run("file.save");

    QCOMPARE(read("second.txt"), QByteArray("second"));
    QCOMPARE(window.windowTitle(), QStringLiteral("second.txt[*] - Scribe"));
    window.commands().run("tab.next");
    QVERIFY(window.isWindowModified()); // the other tab is untouched by the save
}

void MainWindowTest::closeCommandsManageTabs()
{
    MainWindow window(scriptedDialogs());
    window.commands().run("file.new");
    window.commands().run("file.new");
    QCOMPARE(window.tabCount(), 3);

    window.commands().run("tab.close");
    QCOMPARE(window.tabCount(), 2);

    window.commands().run("tab.closeOthers");
    QCOMPARE(window.tabCount(), 1);

    window.commands().run("file.new");
    window.commands().run("tab.closeAll");
    QCOMPARE(window.tabCount(), 1);
    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - Scribe"));
}

void MainWindowTest::closingTheLastTabKeepsTheWindowUsable()
{
    MainWindow window(scriptedDialogs());
    window.commands().run("tab.close");

    QCOMPARE(window.tabCount(), 1);
    QVERIFY(window.editor() != nullptr);
    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - Scribe"));
}

void MainWindowTest::exitClosesTheWindow()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(window.isVisible());

    window.commands().run("file.exit");
    QVERIFY(!window.isVisible());
}

void MainWindowTest::realKeystrokesSwitchTabsWhileTheEditorHasFocus()
{
    MainWindow window(scriptedDialogs());
    window.commands().run("file.new");
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(window.tabCount(), 2);
    QVERIFY(window.editor()->hasFocus());

    // The text editor must not swallow Ctrl+Tab by inserting a tab character.
    QTest::keyClick(window.editor(), Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(window.editor()->toPlainText(), QString());
    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - Scribe"));
    const auto* before = window.editor();

    QTest::keyClick(window.editor(), Qt::Key_Tab, Qt::ControlModifier);
    QVERIFY(window.editor() != before);

    QTest::keyClick(window.editor(), Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(window.editor(), before);
}

void MainWindowTest::closingTheWindowAsksAboutUnsavedChanges()
{
    MainWindow window(scriptedDialogs());
    window.show();
    type(window, "work in progress");

    m_choices = {UnsavedChoice::Cancel};
    window.close();

    QCOMPARE(m_promptedFor, (QStringList{"Untitled"}));
}

void MainWindowTest::cancelKeepsTheWindowOpen()
{
    MainWindow window(scriptedDialogs());
    window.show();
    type(window, "work in progress");

    m_choices = {UnsavedChoice::Cancel};
    QVERIFY(!window.close());

    QVERIFY(window.isVisible());
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("work in progress"));
    QVERIFY(window.isWindowModified());
}

void MainWindowTest::discardClosesTheWindowWithoutSaving()
{
    write("a.txt", "original");
    m_openAnswer = path("a.txt");
    MainWindow window(scriptedDialogs());
    window.show();
    window.commands().run("file.open");
    type(window, " changed");

    m_choices = {UnsavedChoice::Discard};
    QVERIFY(window.close());

    QVERIFY(!window.isVisible());
    QCOMPARE(read("a.txt"), QByteArray("original"));
}

void MainWindowTest::saveWritesTheFileThenClosesTheWindow()
{
    MainWindow window(scriptedDialogs());
    window.show();
    type(window, "keep this");

    m_choices = {UnsavedChoice::Save};
    m_saveAnswer = path("kept.txt");
    QVERIFY(window.close());

    QCOMPARE(read("kept.txt"), QByteArray("keep this"));
    QVERIFY(!window.isVisible());
}

void MainWindowTest::exitCommandAsksToo()
{
    MainWindow window(scriptedDialogs());
    window.show();
    type(window, "x");

    m_choices = {UnsavedChoice::Cancel};
    window.commands().run("file.exit");

    QVERIFY(window.isVisible());
    QCOMPARE(m_promptedFor.size(), 1);
}

void MainWindowTest::cleanWindowClosesWithoutAsking()
{
    MainWindow window(scriptedDialogs());
    window.show();
    window.commands().run("file.new");

    QVERIFY(window.close());
    QVERIFY(m_promptedFor.isEmpty());
}

void MainWindowTest::cancelOnALaterTabKeepsTheWindowAndEarlierClosures()
{
    MainWindow window(scriptedDialogs());
    window.show();
    type(window, "first");
    window.commands().run("file.new");
    type(window, "second");
    window.commands().run("file.new");
    type(window, "third");
    QCOMPARE(window.tabCount(), 3);

    m_choices = {UnsavedChoice::Discard, UnsavedChoice::Cancel};
    QVERIFY(!window.close());

    QVERIFY(window.isVisible());
    QCOMPARE(window.tabCount(), 2);
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("second")); // the refused tab
}

void MainWindowTest::newAndOpenDoNotAsk()
{
    write("a.txt", "abc");
    m_openAnswer = path("a.txt");
    MainWindow window(scriptedDialogs());
    type(window, "unsaved");

    window.commands().run("file.new");
    window.commands().run("file.open");

    QVERIFY(m_promptedFor.isEmpty());
    // New added an empty tab and Open reused it, so the unsaved tab was never touched.
    QCOMPARE(window.tabCount(), 2);
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("abc"));
    window.commands().run("tab.previous");
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("unsaved"));
}

void MainWindowTest::closeTabCommandAsksAboutThatTab()
{
    MainWindow window(scriptedDialogs());
    window.commands().run("file.new");
    type(window, "unsaved");

    m_choices = {UnsavedChoice::Cancel};
    window.commands().run("tab.close");
    QCOMPARE(window.tabCount(), 2);

    m_choices = {UnsavedChoice::Discard};
    window.commands().run("tab.close");
    QCOMPARE(window.tabCount(), 1);
    QCOMPARE(m_promptedFor.size(), 2);
}

void MainWindowTest::editMenuEntriesAreAvailableOnlyWhenTheyCanRun()
{
    MainWindow window(scriptedDialogs());
    QMenu* editMenu = menuTitled(window, "&Edit");
    QVERIFY(editMenu != nullptr);
    QCOMPARE(editMenu->title(), QStringLiteral("&Edit"));
    const auto action = [&](const char* id) { return window.findChild<QAction*>(id); };

    emit editMenu->aboutToShow();               // what Qt does just before the menu appears
    QVERIFY(!action("edit.undo")->isEnabled()); // nothing typed yet
    QVERIFY(!action("edit.copy")->isEnabled());
    QVERIFY(action("edit.selectAll")->isEnabled());
    QVERIFY(action("edit.insertDateTime")->isEnabled());

    type(window, "some text");
    window.editor()->selectAll();
    emit editMenu->aboutToShow();
    QVERIFY(action("edit.undo")->isEnabled());
    QVERIFY(action("edit.cut")->isEnabled());
    QVERIFY(action("edit.copy")->isEnabled());
    QVERIFY(action("edit.delete")->isEnabled());

    window.editor()->moveCursor(QTextCursor::End); // selection gone again
    emit editMenu->aboutToShow();
    QVERIFY(!action("edit.copy")->isEnabled());
    QVERIFY(!action("edit.delete")->isEnabled());
}

void MainWindowTest::editMenuEntriesRunTheirCommands()
{
    MainWindow window(scriptedDialogs());
    type(window, "abc");

    window.findChild<QAction*>("edit.undo")->trigger();
    QCOMPARE(window.editor()->toPlainText(), QString());

    window.findChild<QAction*>("edit.redo")->trigger();
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("abc"));

    window.findChild<QAction*>("edit.selectAll")->trigger();
    QCOMPARE(window.editor()->textCursor().selectedText(), QStringLiteral("abc"));
}

void MainWindowTest::realKeystrokesEditTheText()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(window.editor()->hasFocus());
    QPlainTextEdit* editor = window.editor();

    // The menu's shortcuts must not get in the way of the editor's own key handling.
    QTest::keyClicks(editor, "abcd");
    QCOMPARE(editor->toPlainText(), QStringLiteral("abcd"));

    QTest::keyClick(editor, Qt::Key_Home);
    QTest::keyClick(editor, Qt::Key_Delete); // no selection: must still delete the next character
    QCOMPARE(editor->toPlainText(), QStringLiteral("bcd"));

    QTest::keyClick(editor, Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("bcd"));

    QTest::keyClick(editor, Qt::Key_Delete); // with a selection: removes it
    QCOMPARE(editor->toPlainText(), QString());

    QTest::keyClick(editor, Qt::Key_Z, Qt::ControlModifier);
    QVERIFY(!editor->toPlainText().isEmpty()); // undo brought text back
    QTest::keyClick(editor, Qt::Key_Y, Qt::ControlModifier);

    QTest::keyClick(editor, Qt::Key_F5); // inserts the date and time
    QVERIFY2(editor->toPlainText().contains(QRegularExpression(QStringLiteral("\\d"))) &&
                 editor->toPlainText().size() >= 6,
             qPrintable(editor->toPlainText()));
}

// A real drag enters a widget before it drops, and Qt routes the drop to the widget that accepted
// the enter, so the two events are sent in that order.
void MainWindowTest::dragAndDrop(QWidget* target, const QMimeData& mime)
{
    QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &enter);
    QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &drop);
}

void MainWindowTest::droppingFilesOnTheWindowOpensThemInTabs()
{
    const QString first = path("a.txt");
    const QString second = path("b.txt");
    write("a.txt", "one");
    write("b.txt", "two");
    MainWindow window(scriptedDialogs());

    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(first), QUrl::fromLocalFile(second)});
    dragAndDrop(window.editor()->viewport(), mime);

    QTRY_COMPARE(window.tabCount(), 2); // the drop returns first; the files open right after
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("two")); // the last one is active
    QCOMPARE(window.windowTitle(), QStringLiteral("b.txt[*] - Scribe"));
    QVERIFY(m_promptedFor.isEmpty());
    QVERIFY(m_errors.isEmpty());
}

void MainWindowTest::droppingAFailingFileShowsAnError()
{
    MainWindow window(scriptedDialogs());

    // The file exists when dragged (the drop filter checks) but cannot be opened as text safely.
    const QString bad = path("odd.txt");
    write("odd.txt", QByteArray::fromHex("FFFE6800690000"));
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(bad)});
    dragAndDrop(window.editor()->viewport(), mime);

    QTRY_COMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("odd.txt"), qPrintable(m_errors.first()));
    QCOMPARE(window.tabCount(), 1);
}

void MainWindowTest::viewMenuHasTheViewCommandsWithShortcuts()
{
    MainWindow window(scriptedDialogs());
    const auto action = [&](const char* id) { return window.findChild<QAction*>(id); };

    for (const char* id : {"view.wordWrap", "view.font", "view.zoomIn", "view.zoomOut",
                           "view.zoomReset", "view.statusBar"}) {
        QVERIFY2(action(id) != nullptr, id);
    }

    // Both shortcuts of Zoom In work; the first is the one shown in the menu.
    QCOMPARE(action("view.zoomIn")->shortcuts(),
             (QList<QKeySequence>{QKeySequence(QStringLiteral("Ctrl++")),
                                  QKeySequence(QStringLiteral("Ctrl+="))}));
    QCOMPARE(action("view.zoomOut")->shortcut(), QKeySequence(QStringLiteral("Ctrl+-")));
    QCOMPARE(action("view.zoomReset")->shortcut(), QKeySequence(QStringLiteral("Ctrl+0")));
}

void MainWindowTest::viewMenuChecksFollowWordWrapAndStatusBar()
{
    MainWindow window(scriptedDialogs());
    QMenu* viewMenu = menuTitled(window, "&View");
    QVERIFY(viewMenu != nullptr);
    QCOMPARE(viewMenu->title(), QStringLiteral("&View"));
    const auto action = [&](const char* id) { return window.findChild<QAction*>(id); };

    QVERIFY(action("view.wordWrap")->isCheckable());
    QVERIFY(action("view.statusBar")->isCheckable());
    QVERIFY(!action("view.zoomIn")->isCheckable()); // only toggles get a check mark

    emit viewMenu->aboutToShow();
    QVERIFY(action("view.wordWrap")->isChecked());
    QVERIFY(action("view.statusBar")->isChecked());

    window.editor()->setLineWrapMode(QPlainTextEdit::NoWrap); // out-of-band, to prove it re-reads
    window.commands().run("view.wordWrap");                   // toggles the shared setting
    emit viewMenu->aboutToShow();
    QVERIFY(!action("view.wordWrap")->isChecked());
}

void MainWindowTest::wordWrapMenuEntryTogglesEveryTab()
{
    MainWindow window(scriptedDialogs());
    window.commands().run("file.new");
    QCOMPARE(window.tabCount(), 2);

    window.findChild<QAction*>("view.wordWrap")->trigger();

    QVERIFY(!window.findChild<QAction*>("view.wordWrap")->isChecked());
    QCOMPARE(window.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);
    window.commands().run("tab.previous");
    QCOMPARE(window.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);

    window.findChild<QAction*>("view.wordWrap")->trigger();
    QVERIFY(window.findChild<QAction*>("view.wordWrap")->isChecked());
    QCOMPARE(window.editor()->lineWrapMode(), QPlainTextEdit::WidgetWidth);
}

void MainWindowTest::statusBarMenuEntryHidesAndShowsTheBar()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QVERIFY(bar != nullptr);
    QVERIFY(bar->isVisible());

    window.findChild<QAction*>("view.statusBar")->trigger();
    QVERIFY(!bar->isVisible());
    QVERIFY(!window.findChild<QAction*>("view.statusBar")->isChecked());

    window.findChild<QAction*>("view.statusBar")->trigger();
    QVERIFY(bar->isVisible());
    QVERIFY(window.findChild<QAction*>("view.statusBar")->isChecked());
}

void MainWindowTest::statusBarFollowsTypingAndTabsInTheWindow()
{
    MainWindow window(scriptedDialogs());
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QVERIFY(bar != nullptr);
    QCOMPARE(bar->position(), QStringLiteral("Ln 1, Col 1"));
    QCOMPARE(bar->zoom(), QStringLiteral("100%"));

    type(window, "line one\nline two");
    QCOMPARE(bar->position(), QStringLiteral("Ln 2, Col 9"));
    QCOMPARE(bar->characters(), scribe::editor::EditorStatusBar::charactersText(17));

    window.commands().run("file.new");
    QCOMPARE(bar->position(), QStringLiteral("Ln 1, Col 1")); // the new, empty tab
    QCOMPARE(bar->characters(), scribe::editor::EditorStatusBar::charactersText(0));

    window.commands().run("view.zoomIn");
    QCOMPARE(bar->zoom(), QStringLiteral("110%"));
}

void MainWindowTest::fontMenuEntryAppliesTheChosenFont()
{
    MainWindow window(scriptedDialogs());
    QFont chosen(QStringLiteral("Courier New"));
    chosen.setPointSizeF(15.0);
    m_fontAnswer = chosen;

    window.findChild<QAction*>("view.font")->trigger();

    QCOMPARE(m_fontDialogOpenedWith.size(), 1);
    QCOMPARE(window.editor()->font().family(), QStringLiteral("Courier New"));
    QCOMPARE(window.editor()->font().pointSizeF(), 15.0);

    // The dialog is reopened on the font chosen last time.
    m_fontAnswer = std::nullopt;
    window.findChild<QAction*>("view.font")->trigger();
    QCOMPARE(m_fontDialogOpenedWith.size(), 2);
    QCOMPARE(m_fontDialogOpenedWith.last().family(), QStringLiteral("Courier New"));
    QCOMPARE(window.editor()->font().pointSizeF(), 15.0); // cancelling changed nothing
}

void MainWindowTest::fontMenuEntryIsUnavailableWithoutADialog()
{
    FileDialogs dialogs = scriptedDialogs();
    dialogs.chooseFont = nullptr;
    MainWindow window(dialogs);
    QMenu* viewMenu = menuTitled(window, "&View");
    QVERIFY(viewMenu != nullptr);

    emit viewMenu->aboutToShow();
    QVERIFY(!window.findChild<QAction*>("view.font")->isEnabled());
}

void MainWindowTest::realZoomKeystrokesChangeTheZoomByTenPercent()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(window.editor()->hasFocus());
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QPlainTextEdit* editor = window.editor();

    // Exactly one step per press: the editor's own zoom handling must not add a second one.
    QTest::keyClick(editor, Qt::Key_Plus, Qt::ControlModifier);
    QCOMPARE(bar->zoom(), QStringLiteral("110%"));
    QTest::keyClick(editor, Qt::Key_Equal, Qt::ControlModifier); // Ctrl+= for keyboards without +
    QCOMPARE(bar->zoom(), QStringLiteral("120%"));
    QTest::keyClick(editor, Qt::Key_Minus, Qt::ControlModifier);
    QCOMPARE(bar->zoom(), QStringLiteral("110%"));
    QTest::keyClick(editor, Qt::Key_0, Qt::ControlModifier);
    QCOMPARE(bar->zoom(), QStringLiteral("100%"));

    // The shortcuts must not type anything into the text.
    QCOMPARE(editor->toPlainText(), QString());
}

void MainWindowTest::zoomResetShortcutWorksAfterTheMenuHasBeenOpenedAtOneHundred()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QMenu* viewMenu = menuTitled(window, "&View");
    QVERIFY(viewMenu != nullptr);

    // At 100% "Restore Default Zoom" is unavailable, so the open menu shows it greyed out...
    emit viewMenu->aboutToShow();
    QVERIFY(!window.findChild<QAction*>("view.zoomReset")->isEnabled());
    emit viewMenu->aboutToHide();

    // ...but once the menu is closed that stale state must not block its keyboard shortcut.
    QTest::keyClick(window.editor(), Qt::Key_Plus, Qt::ControlModifier);
    QTest::keyClick(window.editor(), Qt::Key_Plus, Qt::ControlModifier);
    QCOMPARE(bar->zoom(), QStringLiteral("120%"));
    QTest::keyClick(window.editor(), Qt::Key_0, Qt::ControlModifier);
    QCOMPARE(bar->zoom(), QStringLiteral("100%"));
}

void MainWindowTest::ctrlWheelZoomsTheWindow()
{
    MainWindow window(scriptedDialogs());
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QWidget* viewport = window.editor()->viewport();

    QWheelEvent up(QPointF(5, 5), viewport->mapToGlobal(QPointF(5, 5)), QPoint(), QPoint(0, 120),
                   Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(viewport, &up);

    QCOMPARE(bar->zoom(), QStringLiteral("110%"));
}

namespace
{

QToolButton* statusButton(const MainWindow& window, const QString& accessibleName)
{
    for (QToolButton* button : window.findChildren<QToolButton*>()) {
        if (button->accessibleName() == accessibleName) {
            return button;
        }
    }
    return nullptr;
}

QStringList ids(const QList<QAction*>& actions)
{
    QStringList result;
    for (const QAction* action : actions) {
        if (!action->isSeparator()) {
            result << action->objectName();
        }
    }
    return result;
}

} // namespace

void MainWindowTest::encodingMenuHasEveryEncodingAndLineEnding()
{
    MainWindow window(scriptedDialogs());
    QMenu* menu = menuTitled(window, "En&coding");
    QVERIFY(menu != nullptr);
    QCOMPARE(menu->title(), QStringLiteral("En&coding"));

    QCOMPARE(ids(menu->actions()),
             (QStringList{"encoding.utf8", "encoding.utf8Bom", "encoding.utf16Le",
                          "encoding.utf16LeNoBom", "encoding.utf16Be", "encoding.utf16BeNoBom",
                          "encoding.ansi", "lineEnding.crlf", "lineEnding.lf", "lineEnding.cr"}));

    // A separator splits the encodings from the line endings, and every entry is a check item.
    QCOMPARE(menu->actions().size(), 11);
    QVERIFY(menu->actions().at(7)->isSeparator());
    for (const QAction* action : menu->actions()) {
        QVERIFY(action->isSeparator() || action->isCheckable());
    }
    QCOMPARE(window.findChild<QAction*>("encoding.utf8Bom")->text(),
             QStringLiteral("UTF-8 with BOM"));
    QCOMPARE(window.findChild<QAction*>("lineEnding.lf")->text(), QStringLiteral("&Unix (LF)"));
}

void MainWindowTest::encodingMenuChecksFollowTheActiveDocument()
{
    MainWindow window(scriptedDialogs());
    QMenu* menu = menuTitled(window, "En&coding");
    QVERIFY(menu != nullptr);
    const auto checked = [&] {
        emit menu->aboutToShow();
        QStringList result;
        for (const QAction* action : menu->actions()) {
            if (!action->isSeparator() && action->isChecked()) {
                result << action->objectName();
            }
        }
        return result;
    };

    QCOMPARE(checked(), (QStringList{"encoding.utf8", "lineEnding.crlf"}));

    window.findChild<QAction*>("encoding.utf16Be")->trigger();
    window.findChild<QAction*>("lineEnding.cr")->trigger();
    QCOMPARE(checked(), (QStringList{"encoding.utf16Be", "lineEnding.cr"}));

    window.commands().run("file.new"); // the new tab has its own settings
    QCOMPARE(checked(), (QStringList{"encoding.utf8", "lineEnding.crlf"}));
    window.commands().run("tab.previous");
    QCOMPARE(checked(), (QStringList{"encoding.utf16Be", "lineEnding.cr"}));
}

void MainWindowTest::choosingFromTheEncodingMenuUpdatesTheStatusBarAndTab()
{
    MainWindow window(scriptedDialogs());
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QVERIFY(!window.isWindowModified());

    window.findChild<QAction*>("encoding.utf16LeNoBom")->trigger();
    window.findChild<QAction*>("lineEnding.lf")->trigger();

    QCOMPARE(bar->encoding(), QStringLiteral("UTF-16 LE (no BOM)"));
    QCOMPARE(bar->lineEnding(), QStringLiteral("LF"));
    QVERIFY(window.isWindowModified()); // a different format is unsaved work
}

void MainWindowTest::savingAfterChoosingFromTheMenuWritesTheNewFormat()
{
    write("a.txt", "x\ny\n");
    m_openAnswer = path("a.txt");
    MainWindow window(scriptedDialogs());
    window.commands().run("file.open");

    window.findChild<QAction*>("encoding.utf8Bom")->trigger();
    window.findChild<QAction*>("lineEnding.crlf")->trigger();
    window.commands().run("file.save");

    QCOMPARE(read("a.txt"), QByteArray::fromHex("EFBBBF") + "x\r\ny\r\n");
    QVERIFY(!window.isWindowModified());
    QCOMPARE(m_saveAsked, 0); // the file already had a name
}

void MainWindowTest::refusedAnsiShowsAnErrorInTheWindow()
{
    MainWindow window(scriptedDialogs());
    type(window, QStringLiteral("smile \U0001F600"));

    window.findChild<QAction*>("encoding.ansi")->trigger();

    QCOMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("ANSI"), qPrintable(m_errors.first()));
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    QCOMPARE(bar->encoding(), QStringLiteral("UTF-8")); // unchanged
    QVERIFY(!window.findChild<QAction*>("encoding.ansi")->isChecked());
}

void MainWindowTest::clickingTheEncodingSegmentOpensTheEncodingChoices()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QToolButton* button = statusButton(window, "Encoding");
    QVERIFY(button != nullptr);

    button->click();

    auto* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    QVERIFY(popup != nullptr);
    QCOMPARE(ids(popup->actions()),
             (QStringList{"encoding.utf8", "encoding.utf8Bom", "encoding.utf16Le",
                          "encoding.utf16LeNoBom", "encoding.utf16Be", "encoding.utf16BeNoBom",
                          "encoding.ansi"}));
    // It shows the current choice, like the Encoding menu.
    QVERIFY(window.findChild<QAction*>("encoding.utf8")->isChecked());
    popup->close();
}

void MainWindowTest::clickingTheLineEndingSegmentOpensTheLineEndingChoices()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QToolButton* button = statusButton(window, "Line ending");
    QVERIFY(button != nullptr);

    button->click();

    auto* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    QVERIFY(popup != nullptr);
    QCOMPARE(ids(popup->actions()),
             (QStringList{"lineEnding.crlf", "lineEnding.lf", "lineEnding.cr"}));
    QVERIFY(window.findChild<QAction*>("lineEnding.crlf")->isChecked());
    popup->close();
}

void MainWindowTest::choosingFromTheStatusBarPopupChangesTheDocument()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();

    statusButton(window, "Encoding")->click();
    auto* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    QVERIFY(popup != nullptr);
    window.findChild<QAction*>("encoding.utf16Le")->trigger(); // what picking the entry does
    popup->close();

    QCOMPARE(bar->encoding(), QStringLiteral("UTF-16 LE"));
    QVERIFY(window.isWindowModified());
}

void MainWindowTest::statusBarPopupsReuseTheMenuActions()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Opening and closing pop-ups many times must not duplicate or destroy the shared actions.
    for (int i = 0; i < 3; ++i) {
        statusButton(window, "Encoding")->click();
        auto* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        QVERIFY(popup != nullptr);
        popup->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    QCOMPARE(window.findChildren<QAction*>("encoding.ansi").size(), 1);
    QVERIFY(window.findChild<QAction*>("encoding.ansi") != nullptr);
    window.findChild<QAction*>("encoding.ansi")->trigger(); // still works after the pop-ups
    QVERIFY(window.findChild<QAction*>("encoding.ansi")->isChecked());
}

namespace
{

QString hundredLines(int count)
{
    QStringList lines;
    for (int i = 1; i <= count; ++i) {
        lines << QStringLiteral("line %1").arg(i);
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace

void MainWindowTest::searchMenuHasTheFindAndGoToLineCommands()
{
    MainWindow window(scriptedDialogs());
    QMenu* menu = menuTitled(window, "&Search");
    QVERIFY(menu != nullptr);

    QCOMPARE(ids(menu->actions()),
             (QStringList{"search.find", "search.findNext", "search.findPrevious", "search.replace",
                          "search.goToLine"}));
    const QAction* action = window.findChild<QAction*>("search.goToLine");
    QVERIFY(action != nullptr);
    QCOMPARE(action->shortcut(), QKeySequence(QStringLiteral("Ctrl+G")));
    QCOMPARE(action->text(), QStringLiteral("&Go To Line..."));
}

void MainWindowTest::goToLineAsksWithTheCurrentLineAndLineCountThenMovesTheCaret()
{
    MainWindow window(scriptedDialogs());
    window.editor()->setPlainText(hundredLines(300));
    window.editor()->goToLine(12);
    m_lineAnswer = 250;

    window.findChild<QAction*>("search.goToLine")->trigger();

    QCOMPARE(m_lineDialogAskedWith.size(), 1);
    QCOMPARE(m_lineDialogAskedWith.first(), (QPair<int, int>(12, 300)));
    QCOMPARE(window.editor()->currentLine(), 250);
    QCOMPARE(window.editor()->textCursor().positionInBlock(), 0);
}

void MainWindowTest::cancellingGoToLineKeepsTheCaret()
{
    MainWindow window(scriptedDialogs());
    window.editor()->setPlainText(hundredLines(50));
    window.editor()->goToLine(7);
    m_lineAnswer = std::nullopt;

    window.commands().run("search.goToLine");

    QCOMPARE(m_lineDialogAskedWith.size(), 1);
    QCOMPARE(window.editor()->currentLine(), 7);
}

void MainWindowTest::goToLineUpdatesTheStatusBar()
{
    MainWindow window(scriptedDialogs());
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    window.editor()->setPlainText(hundredLines(300));
    m_lineAnswer = 120;

    window.commands().run("search.goToLine");

    QCOMPARE(bar->position(), QStringLiteral("Ln 120, Col 1"));
}

void MainWindowTest::clickingTheLineColumnSegmentOpensGoToLine()
{
    MainWindow window(scriptedDialogs());
    const auto* bar = window.findChild<scribe::editor::EditorStatusBar*>();
    window.editor()->setPlainText(hundredLines(80));
    m_lineAnswer = 33;

    statusButton(window, "Cursor position")->click();

    QCOMPARE(m_lineDialogAskedWith.size(), 1);
    QCOMPARE(m_lineDialogAskedWith.first().second, 80);
    QCOMPARE(window.editor()->currentLine(), 33);
    QCOMPARE(bar->position(), QStringLiteral("Ln 33, Col 1"));
}

void MainWindowTest::realCtrlGOpensGoToLine()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    window.editor()->setPlainText(hundredLines(40));
    m_lineAnswer = 20;

    QTest::keyClick(window.editor(), Qt::Key_G, Qt::ControlModifier);

    QCOMPARE(m_lineDialogAskedWith.size(), 1);
    QCOMPARE(window.editor()->currentLine(), 20);
    QVERIFY(window.editor()->toPlainText().contains(QStringLiteral("line 20"))); // no "g" typed
    QVERIFY(!window.editor()->toPlainText().contains(QLatin1Char('g')));
}

void MainWindowTest::goToLineIsUnavailableWithoutADialog()
{
    FileDialogs dialogs = scriptedDialogs();
    dialogs.askLineNumber = nullptr;
    MainWindow window(dialogs);
    QMenu* menu = menuTitled(window, "&Search");
    QVERIFY(menu != nullptr);

    emit menu->aboutToShow();
    QVERIFY(!window.findChild<QAction*>("search.goToLine")->isEnabled());

    statusButton(window, "Cursor position")->click(); // must not crash or do anything
    QVERIFY(m_lineDialogAskedWith.isEmpty());
}

void MainWindowTest::goToLineActsOnTheActiveTab()
{
    MainWindow window(scriptedDialogs());
    window.editor()->setPlainText(hundredLines(10));
    window.editor()->goToLine(4); // this tab's caret, which must stay put
    window.commands().run("file.new");
    window.editor()->setPlainText(hundredLines(500));
    m_lineAnswer = 400;

    window.commands().run("search.goToLine");

    QCOMPARE(m_lineDialogAskedWith.first().second, 500); // the second tab's line count
    QCOMPARE(window.editor()->currentLine(), 400);
    window.commands().run("tab.previous");
    QCOMPARE(window.editor()->currentLine(), 4); // the other tab kept its own caret
}

namespace
{

QLineEdit* barField(const MainWindow& window, const QString& accessibleName)
{
    for (QLineEdit* edit : window.findBar()->findChildren<QLineEdit*>()) {
        if (edit->accessibleName() == accessibleName) {
            return edit;
        }
    }
    return nullptr;
}

} // namespace

void MainWindowTest::findBarIsHiddenAtStartupAndEditorHasTheFocus()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(window.findBar() != nullptr);
    QVERIFY(!window.findBar()->isOpen());
    QVERIFY(!window.findBar()->isVisible());
    QVERIFY(window.editor()->hasFocus()); // typing works as soon as the window appears
}

void MainWindowTest::searchMenuHasTheShortcuts()
{
    MainWindow window(scriptedDialogs());
    const auto shortcut = [&](const char* id) {
        return window.findChild<QAction*>(id)->shortcut();
    };

    QCOMPARE(shortcut("search.find"), QKeySequence(QStringLiteral("Ctrl+F")));
    QCOMPARE(shortcut("search.findNext"), QKeySequence(QStringLiteral("F3")));
    QCOMPARE(shortcut("search.findPrevious"), QKeySequence(QStringLiteral("Shift+F3")));
    QCOMPARE(shortcut("search.replace"), QKeySequence(QStringLiteral("Ctrl+H")));
}

void MainWindowTest::searchMenuEntriesOpenTheBar()
{
    MainWindow window(scriptedDialogs());

    window.findChild<QAction*>("search.find")->trigger();
    QVERIFY(window.findBar()->isOpen());
    QVERIFY(!window.findBar()->isReplaceRowOpen());

    window.findBar()->closeBar();
    window.findChild<QAction*>("search.replace")->trigger();
    QVERIFY(window.findBar()->isReplaceRowOpen());
}

void MainWindowTest::ctrlFOpensTheBarWithTheFocusInTheFindField()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QTest::keyClick(window.editor(), Qt::Key_F, Qt::ControlModifier);

    QVERIFY(window.findBar()->isOpen());
    QVERIFY(window.findBar()->isVisible());
    QCOMPARE(QApplication::focusWidget(), barField(window, "Find"));
    QVERIFY(window.editor()->toPlainText().isEmpty()); // no "f" was typed into the text
}

void MainWindowTest::typingInTheBarSelectsMatchesInTheEditor()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    window.editor()->setPlainText(QStringLiteral("alpha beta gamma beta"));
    window.editor()->moveCursor(QTextCursor::Start);

    QTest::keyClick(window.editor(), Qt::Key_F, Qt::ControlModifier);
    QTest::keyClicks(barField(window, "Find"), "beta");

    QCOMPARE(window.editor()->textCursor().selectedText(), QStringLiteral("beta"));
    QCOMPARE(window.editor()->textCursor().selectionStart(), 6);
    QCOMPARE(window.findBar()->statusText(), QStringLiteral("1 of 2"));

    QTest::keyClick(barField(window, "Find"), Qt::Key_Return);
    QCOMPARE(window.editor()->textCursor().selectionStart(), 17);
}

void MainWindowTest::f3AndShiftF3NavigateWithTheBarClosed()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    window.editor()->setPlainText(QStringLiteral("x.x.x"));
    window.editor()->moveCursor(QTextCursor::Start);
    window.findBar()->openFind();
    window.findBar()->setQuery(QStringLiteral("x"));
    window.findBar()->closeBar();
    QVERIFY(window.editor()->hasFocus());

    QTest::keyClick(window.editor(), Qt::Key_F3);
    QCOMPARE(window.editor()->textCursor().selectionStart(), 2);
    QTest::keyClick(window.editor(), Qt::Key_F3);
    QCOMPARE(window.editor()->textCursor().selectionStart(), 4);
    QTest::keyClick(window.editor(), Qt::Key_F3, Qt::ShiftModifier);
    QCOMPARE(window.editor()->textCursor().selectionStart(), 2);

    QVERIFY(!window.findBar()->isOpen());                              // the bar stayed closed
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("x.x.x")); // and nothing was typed
}

void MainWindowTest::ctrlHOpensTheReplaceRow()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QTest::keyClick(window.editor(), Qt::Key_H, Qt::ControlModifier);

    QVERIFY(window.findBar()->isReplaceRowOpen());
    QCOMPARE(QApplication::focusWidget(), barField(window, "Find"));
}

void MainWindowTest::escapeReturnsTheFocusToTheEditor()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QTest::keyClick(window.editor(), Qt::Key_F, Qt::ControlModifier);
    QCOMPARE(QApplication::focusWidget(), barField(window, "Find"));

    QTest::keyClick(barField(window, "Find"), Qt::Key_Escape);

    QVERIFY(!window.findBar()->isOpen());
    QVERIFY(window.editor()->hasFocus()); // the caret is back in the text, ready to type
}

void MainWindowTest::replaceAllThenCtrlZRestoresTheTextInOneStep()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    window.editor()->setPlainText(QStringLiteral("cat and cat and cat"));
    window.editor()->document()->clearUndoRedoStacks();
    window.editor()->moveCursor(QTextCursor::Start);

    QTest::keyClick(window.editor(), Qt::Key_H, Qt::ControlModifier);
    window.findBar()->setQuery(QStringLiteral("cat"));
    window.findBar()->setReplacement(QStringLiteral("dog"));
    QCOMPARE(window.findBar()->replaceAll(), 3);
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("dog and dog and dog"));

    QTest::keyClick(barField(window, "Find"), Qt::Key_Escape); // back to the text
    QTest::keyClick(window.editor(), Qt::Key_Z, Qt::ControlModifier);

    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("cat and cat and cat"));
}

void MainWindowTest::replaceAllMarksTheWindowModified()
{
    MainWindow window(scriptedDialogs());
    window.editor()->setPlainText(QStringLiteral("cat cat"));
    window.editor()->document()->setModified(false);
    QVERIFY(!window.isWindowModified());

    window.findBar()->openReplace();
    window.findBar()->setQuery(QStringLiteral("cat"));
    window.findBar()->setReplacement(QStringLiteral("dog"));
    window.findBar()->replaceAll();

    QVERIFY(window.isWindowModified());
    QVERIFY(window.commands().isEnabled("search.replaceAll"));
}

void MainWindowTest::ctrlFTakesTheSelectedTextAsTheQuery()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    window.editor()->setPlainText(QStringLiteral("one two three two"));
    QTextCursor cursor(window.editor()->document());
    cursor.setPosition(4);
    cursor.setPosition(7, QTextCursor::KeepAnchor); // "two"
    window.editor()->setTextCursor(cursor);

    QTest::keyClick(window.editor(), Qt::Key_F, Qt::ControlModifier);

    QCOMPARE(window.findBar()->query(), QStringLiteral("two"));
    QCOMPARE(barField(window, "Find")->text(), QStringLiteral("two"));
}

void MainWindowTest::shortcutsDoNotTypeIntoTheText()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QTest::keyClick(window.editor(), Qt::Key_F, Qt::ControlModifier);
    window.findBar()->closeBar();
    QTest::keyClick(window.editor(), Qt::Key_H, Qt::ControlModifier);
    window.findBar()->closeBar();
    QTest::keyClick(window.editor(), Qt::Key_F3);
    QTest::keyClick(window.editor(), Qt::Key_F3, Qt::ShiftModifier);

    QVERIFY(window.editor()->toPlainText().isEmpty());
}

// ---- Remembering the view settings ----------------------------------------------------------

void MainWindowTest::aWindowWithASettingsStoreStartsWithTheSavedViewSettings()
{
    QFile file(path("scribe.ini"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[view]\nwordWrap=false\nzoom=140\nfontSize=14\n");
    file.close();
    scribe::core::SettingsManager settings(path("scribe.ini"));

    MainWindow window(scriptedDialogs(), &settings);

    QCOMPARE(window.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);
    QCOMPARE(window.editor()->font().pointSizeF(), 14.0 * 140 / 100);
    window.commands().run("file.new"); // and so does a tab opened later
    QCOMPARE(window.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);
}

void MainWindowTest::theStatusBarAndFindOptionsAreRemembered()
{
    {
        scribe::core::SettingsManager settings(path("scribe.ini"));
        MainWindow window(scriptedDialogs(), &settings);
        window.show();
        window.commands().run("view.statusBar"); // hide it
        window.findBar()->setMatchCase(true);
        window.findBar()->setWrapAround(false);
        QVERIFY(window.close());
    }
    scribe::core::SettingsManager settings(path("scribe.ini"));
    MainWindow again(scriptedDialogs(), &settings);
    again.show();
    QVERIFY(again.statusBar()->isHidden());
    QVERIFY(!again.commands().isChecked("view.statusBar"));
    QVERIFY(again.findBar()->matchCase());
    QVERIFY(!again.findBar()->wrapAround());
}

void MainWindowTest::viewChangesAreSavedWhenTheWindowCloses()
{
    scribe::core::SettingsManager settings(path("scribe.ini"));
    MainWindow window(scriptedDialogs(), &settings);
    window.show();

    window.commands().run("view.wordWrap");
    window.commands().run("view.zoomIn");
    window.commands().run("view.zoomIn");
    QVERIFY(window.close()); // closing must not lose a change made just before it

    QFile file(path("scribe.ini"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    QVERIFY2(text.contains(QStringLiteral("wordWrap=false")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("zoom=120")), qPrintable(text));
}

void MainWindowTest::aSecondWindowStartsWhereTheFirstLeftOff()
{
    scribe::core::SettingsManager settings(path("scribe.ini"));
    {
        MainWindow first(scriptedDialogs(), &settings);
        first.commands().run("view.wordWrap");
        first.commands().run("view.zoomIn");
        first.commands().run("view.zoomIn");
        first.commands().run("view.zoomIn");
    } // destroyed, as when the program ends

    MainWindow second(scriptedDialogs(), &settings);

    QCOMPARE(second.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);
    QVERIFY(!second.commands().isChecked("view.wordWrap"));
    const auto* bar = second.findChild<scribe::editor::EditorStatusBar*>();
    QCOMPARE(bar->zoom(), QStringLiteral("130%"));
}

void MainWindowTest::theFontChosenInTheDialogIsRemembered()
{
    scribe::core::SettingsManager settings(path("scribe.ini"));
    QFont chosen(QStringLiteral("Courier New"));
    chosen.setPointSizeF(15.0);
    chosen.setBold(true);
    chosen.setItalic(true);
    {
        MainWindow first(scriptedDialogs(), &settings);
        m_fontAnswer = chosen;
        first.commands().run("view.font");
    }

    MainWindow second(scriptedDialogs(), &settings);

    QCOMPARE(second.editor()->font().pointSizeF(), 15.0);
    QVERIFY(second.editor()->font().italic());
    QVERIFY(second.editor()->font().bold());
}

void MainWindowTest::zoomFromTheMouseWheelIsRemembered()
{
    scribe::core::SettingsManager settings(path("scribe.ini"));
    {
        MainWindow first(scriptedDialogs(), &settings);
        QWidget* viewport = first.editor()->viewport();
        for (int i = 0; i < 5; ++i) { // five notches, one after the other
            QWheelEvent wheel(QPointF(5, 5), viewport->mapToGlobal(QPointF(5, 5)), QPoint(),
                              QPoint(0, 120), Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase,
                              false);
            QApplication::sendEvent(viewport, &wheel);
        }
    }

    MainWindow second(scriptedDialogs(), &settings);
    const auto* bar = second.findChild<scribe::editor::EditorStatusBar*>();
    QCOMPARE(bar->zoom(), QStringLiteral("150%"));
}

void MainWindowTest::theStatusBarShowsTheSavedZoomFromTheStart()
{
    QFile file(path("scribe.ini"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[view]\nzoom=180\n");
    file.close();
    scribe::core::SettingsManager settings(path("scribe.ini"));

    MainWindow window(scriptedDialogs(), &settings);

    QCOMPARE(window.findChild<scribe::editor::EditorStatusBar*>()->zoom(), QStringLiteral("180%"));
}

void MainWindowTest::aWindowWithoutAStoreStillWorksAndSavesNothing()
{
    MainWindow window(scriptedDialogs()); // no settings store
    window.commands().run("view.zoomIn");
    window.commands().run("view.wordWrap");

    QCOMPARE(window.findChild<scribe::editor::EditorStatusBar*>()->zoom(), QStringLiteral("110%"));
    QVERIFY(!QFileInfo::exists(path("scribe.ini")));
}

void MainWindowTest::aBrokenSettingsFileStillGivesAWorkingWindow()
{
    QFile file(path("scribe.ini"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("\xFF\xFE garbage \x00\x01 [view\nzoom=banana\nwordWrap=\n=\n[[[");
    file.close();
    scribe::core::SettingsManager settings(path("scribe.ini"));

    MainWindow window(scriptedDialogs(), &settings);

    QCOMPARE(window.findChild<scribe::editor::EditorStatusBar*>()->zoom(), QStringLiteral("100%"));
    QCOMPARE(window.editor()->lineWrapMode(), QPlainTextEdit::WidgetWidth); // the default
    window.editor()->insertPlainText(QStringLiteral("still usable"));
    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("still usable"));
}

QTEST_MAIN(MainWindowTest)

#include "MainWindowTest.moc"
