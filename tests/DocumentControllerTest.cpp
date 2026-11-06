#include "DocumentController.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;

// The unsaved-changes prompt: every choice, for every way a modified document can be closed.
class DocumentControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void cleanTabClosesWithoutAsking();
    void promptNamesTheDocument();
    void discardClosesTheTabAndLeavesTheFileAlone();
    void cancelKeepsTheTabAndItsText();
    void saveWritesTheFileThenCloses();
    void saveOfAnUntitledTabAsksForAPathThenCloses();
    void cancelledSaveAsKeepsTheTab();
    void failedSaveKeepsTheTabAndShowsAnError();
    void unencodableTextKeepsTheTabAndNamesTheEncoding();
    void missingPromptFunctionKeepsTheTab();
    void modifiedTabsAreShownWhenAsked();
    void closeOthersAsksOnlyAboutTheOthers();
    void closeAllStopsAtCancelAndKeepsEarlierClosures();
    void closeAllMixesSaveDiscardAndCleanTabs();
    void newAndOpenNeverAsk();

    void openFilesOpensEachFileInOrderAndLeavesTheLastActive();
    void openFilesShowsOneErrorPerFailureAndStillOpensTheRest();
    void openFilesSwitchesToFilesThatAreAlreadyOpen();
    void openFilesWithNoPathsDoesNothing();
    void openFilesNeverAsksAboutUnsavedChanges();

private:
    QString path(const QString& name) const { return m_dir->filePath(name); }
    QString write(const QString& name, const QByteArray& bytes) const;
    QByteArray read(const QString& name) const;
    static void edit(EditorWidget* editor, const QString& text = QStringLiteral("edit"));

    // Scripted answers, consumed in order. When they run out the answer is Cancel.
    FileDialogs scriptedDialogs();
    QList<UnsavedChoice> m_choices;
    QStringList m_askedAbout;
    QList<int> m_activeIndexWhenAsked;
    QString m_openAnswer;
    QString m_saveAnswer;
    QStringList m_errors;
    EditorTabs* m_tabsUnderTest = nullptr;

    std::unique_ptr<QTemporaryDir> m_dir;
};

void DocumentControllerTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_choices.clear();
    m_askedAbout.clear();
    m_activeIndexWhenAsked.clear();
    m_openAnswer.clear();
    m_saveAnswer.clear();
    m_errors.clear();
    m_tabsUnderTest = nullptr;
}

FileDialogs DocumentControllerTest::scriptedDialogs()
{
    FileDialogs dialogs;
    dialogs.chooseOpenPath = [this](QWidget*) { return m_openAnswer; };
    dialogs.chooseSavePath = [this](QWidget*, const QString&) { return m_saveAnswer; };
    dialogs.showError = [this](QWidget*, const QString&, const QString& message) {
        m_errors.append(message);
    };
    dialogs.confirmDiscard = [this](QWidget*, const QString& name) {
        m_askedAbout.append(name);
        m_activeIndexWhenAsked.append(m_tabsUnderTest != nullptr ? m_tabsUnderTest->currentIndex()
                                                                 : -1);
        return m_choices.isEmpty() ? UnsavedChoice::Cancel : m_choices.takeFirst();
    };
    return dialogs;
}

QString DocumentControllerTest::write(const QString& name, const QByteArray& bytes) const
{
    QFile file(path(name));
    file.open(QIODevice::WriteOnly);
    file.write(bytes);
    return path(name);
}

QByteArray DocumentControllerTest::read(const QString& name) const
{
    QFile file(path(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void DocumentControllerTest::edit(EditorWidget* editor, const QString& text)
{
    editor->moveCursor(QTextCursor::End);
    editor->insertPlainText(text);
}

void DocumentControllerTest::cleanTabClosesWithoutAsking()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    tabs.addEditor();

    QVERIFY(tabs.closeTab(1));
    QVERIFY(tabs.closeAllTabs());
    QVERIFY(m_askedAbout.isEmpty());
}

void DocumentControllerTest::promptNamesTheDocument()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);

    QVERIFY(tabs.openFile(write("notes.txt", "abc")).ok());
    edit(tabs.currentEditor());
    tabs.closeTab(0); // Cancel (no scripted answer)
    tabs.addEditor();
    edit(tabs.currentEditor());
    tabs.closeTab(1);

    QCOMPARE(m_askedAbout, (QStringList{"notes.txt", "Untitled"}));
}

void DocumentControllerTest::discardClosesTheTabAndLeavesTheFileAlone()
{
    const QString file = write("a.txt", "original");
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    QVERIFY(tabs.openFile(file).ok());
    edit(tabs.currentEditor());
    tabs.addEditor();

    m_choices = {UnsavedChoice::Discard};
    QVERIFY(tabs.closeTab(0));

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(read("a.txt"), QByteArray("original"));
}

void DocumentControllerTest::cancelKeepsTheTabAndItsText()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    edit(tabs.currentEditor(), "precious");

    m_choices = {UnsavedChoice::Cancel};
    QVERIFY(!tabs.closeTab(0));

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("precious"));
    QVERIFY(tabs.currentEditor()->isModified());
}

void DocumentControllerTest::saveWritesTheFileThenCloses()
{
    const QString file = write("a.txt", "abc");
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    QVERIFY(tabs.openFile(file).ok());
    edit(tabs.currentEditor(), "def");

    m_choices = {UnsavedChoice::Save};
    QVERIFY(tabs.closeTab(0));

    QCOMPARE(read("a.txt"), QByteArray("abcdef"));
    QCOMPARE(tabs.count(), 1);
    QVERIFY(m_errors.isEmpty());
}

void DocumentControllerTest::saveOfAnUntitledTabAsksForAPathThenCloses()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    edit(tabs.currentEditor(), "new text");

    m_choices = {UnsavedChoice::Save};
    m_saveAnswer = path("fresh.txt");
    QVERIFY(tabs.closeTab(0));

    QCOMPARE(read("fresh.txt"), QByteArray("new text"));
    QCOMPARE(tabs.count(), 1);
    QVERIFY(tabs.currentEditor()->fileDocument().filePath().isEmpty()); // the fresh tab
}

void DocumentControllerTest::cancelledSaveAsKeepsTheTab()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    edit(tabs.currentEditor(), "keep me");

    m_choices = {UnsavedChoice::Save};
    m_saveAnswer.clear(); // the user cancels the Save As dialog
    QVERIFY(!tabs.closeTab(0));

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("keep me"));
    QVERIFY(tabs.currentEditor()->isModified());
    QVERIFY(m_errors.isEmpty());
}

void DocumentControllerTest::failedSaveKeepsTheTabAndShowsAnError()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    edit(tabs.currentEditor(), "keep me");

    m_choices = {UnsavedChoice::Save};
    m_saveAnswer = path("no-such-folder/out.txt");
    QVERIFY(!tabs.closeTab(0));

    QCOMPARE(tabs.count(), 1);
    QVERIFY(tabs.currentEditor()->isModified());
    QCOMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("out.txt"), qPrintable(m_errors.first()));
}

void DocumentControllerTest::unencodableTextKeepsTheTabAndNamesTheEncoding()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    tabs.currentEditor()->setEncoding({scribe::core::Encoding::Ansi, false});
    edit(tabs.currentEditor(), QStringLiteral("smile \U0001F600"));

    m_choices = {UnsavedChoice::Save};
    m_saveAnswer = path("out.txt");
    QVERIFY(!tabs.closeTab(0));

    QCOMPARE(m_errors.size(), 1);
    QVERIFY2(m_errors.first().contains("ANSI"), qPrintable(m_errors.first()));
    QVERIFY(!QFile::exists(path("out.txt")));
    QCOMPARE(tabs.count(), 1);
}

void DocumentControllerTest::missingPromptFunctionKeepsTheTab()
{
    FileDialogs dialogs = scriptedDialogs();
    dialogs.confirmDiscard = nullptr; // nothing can ask the user
    EditorTabs tabs;
    DocumentController controller(tabs, dialogs, nullptr);
    edit(tabs.currentEditor());

    QVERIFY(!tabs.closeTab(0));
    QVERIFY(!tabs.closeAllTabs());
    QVERIFY(tabs.currentEditor()->isModified());
}

void DocumentControllerTest::modifiedTabsAreShownWhenAsked()
{
    EditorTabs tabs;
    m_tabsUnderTest = &tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    tabs.addEditor();
    tabs.addEditor();
    for (int i = 0; i < 3; ++i) {
        edit(tabs.editorAt(i));
    }
    tabs.setCurrentIndex(2);

    m_choices = {UnsavedChoice::Discard, UnsavedChoice::Discard, UnsavedChoice::Discard};
    QVERIFY(tabs.closeAllTabs());

    // Tab 2 was active at the start, yet the first question came while tab 0 was active: each
    // tab is shown before it is asked about. Approved tabs are removed, so the next one is at 0.
    QCOMPARE(m_askedAbout.size(), 3);
    QCOMPARE(m_activeIndexWhenAsked, (QList<int>{0, 0, 0}));
}

void DocumentControllerTest::closeOthersAsksOnlyAboutTheOthers()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    tabs.addEditor();
    tabs.addEditor();
    edit(tabs.editorAt(0), "a");
    edit(tabs.editorAt(1), "b");
    edit(tabs.editorAt(2), "c");
    EditorWidget* keep = tabs.editorAt(1);

    m_choices = {UnsavedChoice::Discard, UnsavedChoice::Discard};
    QVERIFY(tabs.closeOtherTabs(1));

    QCOMPARE(m_askedAbout.size(), 2); // the kept tab was not asked about
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor(), keep);
    QCOMPARE(keep->toPlainText(), QStringLiteral("b"));
}

void DocumentControllerTest::closeAllStopsAtCancelAndKeepsEarlierClosures()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    tabs.addEditor();
    tabs.addEditor();
    edit(tabs.editorAt(0), "one");
    edit(tabs.editorAt(1), "two");
    edit(tabs.editorAt(2), "three");

    m_choices = {UnsavedChoice::Discard, UnsavedChoice::Cancel};
    QVERIFY(!tabs.closeAllTabs());

    QCOMPARE(m_askedAbout.size(), 2); // never asked about the third tab
    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.editorAt(0)->toPlainText(), QStringLiteral("two"));
    QCOMPARE(tabs.editorAt(1)->toPlainText(), QStringLiteral("three"));
    QVERIFY(tabs.editorAt(0)->isModified());
}

void DocumentControllerTest::closeAllMixesSaveDiscardAndCleanTabs()
{
    const QString file = write("saved.txt", "base");
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    QVERIFY(tabs.openFile(file).ok());
    edit(tabs.currentEditor(), "+more");
    tabs.addEditor();
    edit(tabs.currentEditor(), "thrown away");
    tabs.addEditor(); // stays clean

    m_choices = {UnsavedChoice::Save, UnsavedChoice::Discard};
    QVERIFY(tabs.closeAllTabs());

    QCOMPARE(m_askedAbout, (QStringList{"saved.txt", "Untitled"}));
    QCOMPARE(read("saved.txt"), QByteArray("base+more"));
    QCOMPARE(tabs.count(), 1);
    QVERIFY(!tabs.currentEditor()->isModified());
}

void DocumentControllerTest::newAndOpenNeverAsk()
{
    write("a.txt", "abc");
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    edit(tabs.currentEditor(), "unsaved work");

    controller.newFile();
    m_openAnswer = path("a.txt");
    controller.openFile();

    QVERIFY(m_askedAbout.isEmpty());
    // New added an empty tab and Open reused it, so the unsaved tab was never touched.
    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.editorAt(0)->toPlainText(), QStringLiteral("unsaved work"));
    QVERIFY(tabs.editorAt(0)->isModified());
    QCOMPARE(tabs.editorAt(1)->toPlainText(), QStringLiteral("abc"));
}

void DocumentControllerTest::openFilesOpensEachFileInOrderAndLeavesTheLastActive()
{
    const QStringList files = {write("a.txt", "one"), write("b.txt", "two"),
                               write("c.txt", "three")};
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);

    controller.openFiles(files);

    QCOMPARE(tabs.count(), 3); // the first file reused the untouched Untitled tab
    QCOMPARE(tabs.editorAt(0)->toPlainText(), QStringLiteral("one"));
    QCOMPARE(tabs.editorAt(1)->toPlainText(), QStringLiteral("two"));
    QCOMPARE(tabs.editorAt(2)->toPlainText(), QStringLiteral("three"));
    QCOMPARE(tabs.currentIndex(), 2);
    QVERIFY(m_errors.isEmpty());
}

void DocumentControllerTest::openFilesShowsOneErrorPerFailureAndStillOpensTheRest()
{
    // Odd-length UTF-16 cannot be opened without changing it, and a missing file cannot be read.
    const QString bad = write("bad.txt", QByteArray::fromHex("FFFE6800690000"));
    const QString good = write("good.txt", "fine");
    const QString missing = path("missing.txt");
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);

    controller.openFiles({bad, good, missing});

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("fine"));
    QCOMPARE(m_errors.size(), 2); // one dialog per failed file
    QVERIFY2(m_errors.at(0).contains("bad.txt"), qPrintable(m_errors.at(0)));
    QVERIFY2(m_errors.at(1).contains("missing.txt"), qPrintable(m_errors.at(1)));
}

void DocumentControllerTest::openFilesSwitchesToFilesThatAreAlreadyOpen()
{
    const QString first = write("a.txt", "one");
    const QString second = write("b.txt", "two");
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    controller.openFiles({first, second});
    edit(tabs.editorAt(0), "+edited");

    controller.openFiles({first});

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.currentIndex(), 0);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("one+edited")); // edits kept
}

void DocumentControllerTest::openFilesWithNoPathsDoesNothing()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);

    controller.openFiles({});

    QCOMPARE(tabs.count(), 1);
    QVERIFY(m_errors.isEmpty());
    QVERIFY(m_askedAbout.isEmpty());
}

void DocumentControllerTest::openFilesNeverAsksAboutUnsavedChanges()
{
    EditorTabs tabs;
    DocumentController controller(tabs, scriptedDialogs(), nullptr);
    edit(tabs.currentEditor(), "unsaved work");

    controller.openFiles({write("a.txt", "one"), write("b.txt", "two")});

    QVERIFY(m_askedAbout.isEmpty());
    QCOMPARE(tabs.count(), 3);
    QCOMPARE(tabs.editorAt(0)->toPlainText(), QStringLiteral("unsaved work"));
    QVERIFY(tabs.editorAt(0)->isModified());
}

QTEST_MAIN(DocumentControllerTest)

#include "DocumentControllerTest.moc"
