#include "MainWindow.h"

#include "EditorWidget.h"

#include <QAction>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#include <memory>

using scribe::editor::EditorWidget;

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
    QCOMPARE(menus.size(), 3);
    QCOMPARE(menus.at(0)->text(), QStringLiteral("&File"));
    QCOMPARE(menus.at(1)->text(), QStringLiteral("&Edit"));
    QCOMPARE(menus.at(2)->text(), QStringLiteral("&View"));

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
    QMenu* editMenu = window.findChildren<QMenu*>().at(1); // File, Edit, View
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

QTEST_MAIN(MainWindowTest)

#include "MainWindowTest.moc"
