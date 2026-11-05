#include "MainWindow.h"

#include "EditorWidget.h"

#include <QAction>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QTemporaryDir>
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
    void newResetsTheWindow();
    void exitClosesTheWindow();

private:
    QString path(const QString& name) const { return m_dir->filePath(name); }
    void write(const QString& name, const QByteArray& bytes) const;
    QByteArray read(const QString& name) const;
    void type(MainWindow& window, const QString& text) const;

    // Scripted answers for the dialogs, plus a record of what was asked.
    FileDialogs scriptedDialogs();
    QString m_openAnswer;
    QString m_saveAnswer;
    int m_openAsked = 0;
    int m_saveAsked = 0;
    QStringList m_errors;

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
    QCOMPARE(menus.size(), 1);
    QCOMPARE(menus.first()->text(), QStringLiteral("&File"));

    // Menu entries must be the registry's commands, with the registry's shortcuts.
    const QStringList ids = {"file.new", "file.open", "file.save", "file.saveAs", "file.exit"};
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

void MainWindowTest::newResetsTheWindow()
{
    write("a.txt", "abc");
    m_openAnswer = path("a.txt");
    MainWindow window(scriptedDialogs());
    window.commands().run("file.open");
    type(window, "x");

    window.commands().run("file.new");

    QVERIFY(window.editor()->toPlainText().isEmpty());
    QCOMPARE(window.windowTitle(), QStringLiteral("Untitled[*] - Scribe"));
    QVERIFY(!window.isWindowModified());
}

void MainWindowTest::exitClosesTheWindow()
{
    MainWindow window(scriptedDialogs());
    window.show();
    QVERIFY(window.isVisible());

    window.commands().run("file.exit");
    QVERIFY(!window.isVisible());
}

QTEST_MAIN(MainWindowTest)

#include "MainWindowTest.moc"
