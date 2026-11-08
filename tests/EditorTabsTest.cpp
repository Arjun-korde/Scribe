#include "EditorTabs.h"

#include "EditorWidget.h"

#include <QFile>
#include <QSignalSpy>
#include <QTabBar>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;

class EditorTabsTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void startsWithOneUntitledTab();
    void addEditorAppendsAndActivates();
    void modifiedTabShowsAStar();
    void tabTextEscapesAmpersands();
    void tabToolTipShowsTheFullPath();
    void openReusesAnUntouchedUntitledTab();
    void openAddsATabWhenTheCurrentOneHasEdits();
    void openAddsATabWhenTheCurrentOneHasAFile();
    void openingAnOpenFileSwitchesToItAndKeepsEdits();
    void failedOpenAddsNoTab();
    void failedOpenIntoUntouchedTabKeepsIt();
    void fileUsedIsEmittedForOpensInEveryKindOfTab();
    void fileUsedIsNotEmittedWhenOpeningFails();
    void fileUsedIsEmittedForSaveAsButNotForSave();
    void fileUsedIsEmittedForSaveAsInEveryTab();
    void fileUsedIsNotEmittedForNewTabs();
    void closeTabRemovesIt();
    void closingTheLastTabLeavesAFreshUntitled();
    void closeOthersKeepsOnlyTheChosenTab();
    void closeAllLeavesOneFreshUntitled();
    void closeWithInvalidIndexDoesNothing();
    void nextAndPreviousWrapAround();
    void cyclingWithOneTabDoesNothing();
    void stateSignalIsEmittedForTheCurrentTabOnly();
    void stateSignalIsEmittedWhenTheTabChanges();
    void middleClickClosesATab();
    void closeRequestFromTheTabButtonClosesATab();

    void guardIsNotAskedAboutCleanTabs();
    void guardRefusalKeepsTheTab();
    void guardApprovalClosesTheTab();
    void guardSeesTheTabAlreadyActive();
    void closeOthersDoesNotAskAboutTheKeptTab();
    void closeAllStopsAtTheFirstRefusal();
    void closeAllAsksLeftToRight();
    void tabButtonAndMiddleClickUseTheGuard();
    void invalidIndexReturnsFalse();

private:
    QString path(const QString& name) const { return m_dir->filePath(name); }
    QString write(const QString& name, const QByteArray& bytes) const;

    std::unique_ptr<QTemporaryDir> m_dir;
};

void EditorTabsTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

QString EditorTabsTest::write(const QString& name, const QByteArray& bytes) const
{
    QFile file(path(name));
    file.open(QIODevice::WriteOnly);
    file.write(bytes);
    return path(name);
}

void EditorTabsTest::startsWithOneUntitledTab()
{
    EditorTabs tabs;
    QCOMPARE(tabs.count(), 1);
    QVERIFY(tabs.currentEditor() != nullptr);
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled"));
}

void EditorTabsTest::addEditorAppendsAndActivates()
{
    EditorTabs tabs;
    EditorWidget* added = tabs.addEditor();

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.currentEditor(), added);
    QCOMPARE(tabs.editorAt(1), added);
    QCOMPARE(tabs.editorAt(5), nullptr);
}

void EditorTabsTest::modifiedTabShowsAStar()
{
    EditorTabs tabs;
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled"));

    tabs.currentEditor()->insertPlainText(QStringLiteral("x"));
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled *"));

    QVERIFY(tabs.currentEditor()->saveFileAs(path("s.txt")).ok());
    QCOMPARE(tabs.tabText(0), QStringLiteral("s.txt"));
}

void EditorTabsTest::tabTextEscapesAmpersands()
{
    EditorTabs tabs;
    QVERIFY(tabs.openFile(write("a&b.txt", "x")).ok());
    QCOMPARE(tabs.tabText(0), QStringLiteral("a&&b.txt"));
}

void EditorTabsTest::tabToolTipShowsTheFullPath()
{
    EditorTabs tabs;
    const QString file = write("t.txt", "x");
    QVERIFY(tabs.openFile(file).ok());
    QCOMPARE(tabs.tabToolTip(0), QDir::toNativeSeparators(file));
}

void EditorTabsTest::openReusesAnUntouchedUntitledTab()
{
    EditorTabs tabs;
    QVERIFY(tabs.openFile(write("a.txt", "one")).ok());

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("one"));
}

void EditorTabsTest::openAddsATabWhenTheCurrentOneHasEdits()
{
    EditorTabs tabs;
    tabs.currentEditor()->insertPlainText(QStringLiteral("my notes"));

    QVERIFY(tabs.openFile(write("a.txt", "one")).ok());

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.currentIndex(), 1);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("one"));
    QCOMPARE(tabs.editorAt(0)->toPlainText(), QStringLiteral("my notes"));
}

void EditorTabsTest::openAddsATabWhenTheCurrentOneHasAFile()
{
    EditorTabs tabs;
    QVERIFY(tabs.openFile(write("a.txt", "one")).ok());
    QVERIFY(tabs.openFile(write("b.txt", "two")).ok());

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.editorAt(0)->toPlainText(), QStringLiteral("one"));
    QCOMPARE(tabs.editorAt(1)->toPlainText(), QStringLiteral("two"));
}

void EditorTabsTest::openingAnOpenFileSwitchesToItAndKeepsEdits()
{
    EditorTabs tabs;
    const QString first = write("a.txt", "one");
    QVERIFY(tabs.openFile(first).ok());
    tabs.currentEditor()->insertPlainText(QStringLiteral(" edited"));
    QVERIFY(tabs.openFile(write("b.txt", "two")).ok());
    QCOMPARE(tabs.currentIndex(), 1);

    QVERIFY(tabs.openFile(first).ok());

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.currentIndex(), 0);
    QVERIFY(tabs.currentEditor()->toPlainText().contains(QStringLiteral("edited")));
    QVERIFY(tabs.currentEditor()->isModified());
}

void EditorTabsTest::failedOpenAddsNoTab()
{
    EditorTabs tabs;
    tabs.currentEditor()->insertPlainText(QStringLiteral("keep"));

    const auto result = tabs.openFile(path("missing.txt"));

    QVERIFY(!result.ok());
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("keep"));
}

void EditorTabsTest::failedOpenIntoUntouchedTabKeepsIt()
{
    EditorTabs tabs;
    QVERIFY(!tabs.openFile(path("missing.txt")).ok());

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled"));
}

void EditorTabsTest::fileUsedIsEmittedForOpensInEveryKindOfTab()
{
    EditorTabs tabs;
    QSignalSpy used(&tabs, &EditorTabs::fileUsed);

    const QString first = write("a.txt", "one");
    QVERIFY(tabs.openFile(first).ok()); // reuses the untouched Untitled tab
    QCOMPARE(used.count(), 1);
    QCOMPARE(used.at(0).at(0).toString(), first);

    const QString second = write("b.txt", "two");
    QVERIFY(tabs.openFile(second).ok()); // needs a new tab
    QCOMPARE(used.count(), 2);
    QCOMPARE(used.at(1).at(0).toString(), second);
    QCOMPARE(tabs.count(), 2);

    QVERIFY(tabs.openFile(first).ok()); // switches to the tab that already has it
    QCOMPARE(used.count(), 3);
    QCOMPARE(used.at(2).at(0).toString(), first);
    QCOMPARE(tabs.count(), 2);
}

void EditorTabsTest::fileUsedIsNotEmittedWhenOpeningFails()
{
    EditorTabs tabs;
    QSignalSpy used(&tabs, &EditorTabs::fileUsed);
    QVERIFY(!tabs.openFile(path("missing.txt")).ok());
    tabs.currentEditor()->insertPlainText(QStringLiteral("edits")); // so a new tab would be needed
    QVERIFY(!tabs.openFile(path("missing.txt")).ok());
    QCOMPARE(used.count(), 0);
}

void EditorTabsTest::fileUsedIsEmittedForSaveAsButNotForSave()
{
    EditorTabs tabs;
    QSignalSpy used(&tabs, &EditorTabs::fileUsed);
    tabs.currentEditor()->insertPlainText(QStringLiteral("text"));

    const QString target = path("saved.txt");
    QVERIFY(tabs.currentEditor()->saveFileAs(target).ok());
    QCOMPARE(used.count(), 1);
    QCOMPARE(used.at(0).at(0).toString(), target);

    tabs.currentEditor()->insertPlainText(QStringLiteral(" more"));
    QVERIFY(tabs.currentEditor()->saveFile().ok());
    QCOMPARE(used.count(), 1); // a plain Save is not "using" a file again
}

void EditorTabsTest::fileUsedIsEmittedForSaveAsInEveryTab()
{
    EditorTabs tabs;
    QVERIFY(tabs.openFile(write("a.txt", "one")).ok());
    tabs.addEditor(); // a second tab, not the first one
    QSignalSpy used(&tabs, &EditorTabs::fileUsed);

    const QString target = path("second.txt");
    QVERIFY(tabs.currentEditor()->saveFileAs(target).ok());
    QCOMPARE(used.count(), 1);
    QCOMPARE(used.at(0).at(0).toString(), target);
}

void EditorTabsTest::fileUsedIsNotEmittedForNewTabs()
{
    EditorTabs tabs;
    QSignalSpy used(&tabs, &EditorTabs::fileUsed);
    tabs.addEditor();
    tabs.currentEditor()->newFile();
    QCOMPARE(used.count(), 0);
}

void EditorTabsTest::closeTabRemovesIt()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    EditorWidget* middle = tabs.editorAt(1);

    tabs.closeTab(0);

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.editorAt(0), middle);
}

void EditorTabsTest::closingTheLastTabLeavesAFreshUntitled()
{
    EditorTabs tabs;
    QVERIFY(tabs.openFile(write("a.txt", "one")).ok());

    tabs.closeTab(0);

    QCOMPARE(tabs.count(), 1);
    QVERIFY(tabs.currentEditor() != nullptr);
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled"));
    QVERIFY(tabs.currentEditor()->toPlainText().isEmpty());
}

void EditorTabsTest::closeOthersKeepsOnlyTheChosenTab()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    EditorWidget* keep = tabs.editorAt(1);

    tabs.closeOtherTabs(1);

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor(), keep);
}

void EditorTabsTest::closeAllLeavesOneFreshUntitled()
{
    EditorTabs tabs;
    tabs.currentEditor()->insertPlainText(QStringLiteral("x"));
    tabs.addEditor();
    tabs.addEditor();

    tabs.closeAllTabs();

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled"));
    QVERIFY(!tabs.currentEditor()->isModified());
}

void EditorTabsTest::closeWithInvalidIndexDoesNothing()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.closeTab(-1);
    tabs.closeTab(2);
    tabs.closeOtherTabs(7);
    QCOMPARE(tabs.count(), 2);
}

void EditorTabsTest::nextAndPreviousWrapAround()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    tabs.setCurrentIndex(0);

    tabs.nextTab();
    QCOMPARE(tabs.currentIndex(), 1);
    tabs.nextTab();
    tabs.nextTab();
    QCOMPARE(tabs.currentIndex(), 0); // wrapped from the last tab

    tabs.previousTab();
    QCOMPARE(tabs.currentIndex(), 2); // wrapped from the first tab
}

void EditorTabsTest::cyclingWithOneTabDoesNothing()
{
    EditorTabs tabs;
    tabs.nextTab();
    tabs.previousTab();
    QCOMPARE(tabs.currentIndex(), 0);
}

void EditorTabsTest::stateSignalIsEmittedForTheCurrentTabOnly()
{
    EditorTabs tabs;
    tabs.addEditor();
    QSignalSpy spy(&tabs, &EditorTabs::currentEditorStateChanged);

    tabs.editorAt(0)->insertPlainText(QStringLiteral("background"));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(tabs.tabText(0), QStringLiteral("Untitled *")); // the label still updates

    tabs.editorAt(1)->insertPlainText(QStringLiteral("foreground"));
    QVERIFY(spy.count() >= 1);
}

void EditorTabsTest::stateSignalIsEmittedWhenTheTabChanges()
{
    EditorTabs tabs;
    tabs.addEditor();
    QSignalSpy spy(&tabs, &EditorTabs::currentEditorStateChanged);

    tabs.setCurrentIndex(0);
    QCOMPARE(spy.count(), 1);
}

void EditorTabsTest::middleClickClosesATab()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    tabs.resize(600, 300);
    tabs.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabs));

    QTabBar* bar = tabs.tabBar();
    QTest::mouseClick(bar, Qt::MiddleButton, Qt::NoModifier, bar->tabRect(1).center());

    QCOMPARE(tabs.count(), 2);
}

void EditorTabsTest::closeRequestFromTheTabButtonClosesATab()
{
    EditorTabs tabs;
    tabs.addEditor();
    emit tabs.tabCloseRequested(0);
    QCOMPARE(tabs.count(), 1);
}

namespace
{

void edit(EditorWidget* editor)
{
    editor->insertPlainText(QStringLiteral("x"));
}

} // namespace

void EditorTabsTest::guardIsNotAskedAboutCleanTabs()
{
    EditorTabs tabs;
    tabs.addEditor();
    int asked = 0;
    tabs.setCloseGuard([&asked](EditorWidget&) {
        ++asked;
        return false;
    });

    QVERIFY(tabs.closeTab(0));
    QVERIFY(tabs.closeAllTabs());
    QCOMPARE(asked, 0);
}

void EditorTabsTest::guardRefusalKeepsTheTab()
{
    EditorTabs tabs;
    edit(tabs.currentEditor());
    tabs.setCloseGuard([](EditorWidget&) { return false; });

    QVERIFY(!tabs.closeTab(0));
    QCOMPARE(tabs.count(), 1);
    QVERIFY(tabs.currentEditor()->isModified());
}

void EditorTabsTest::guardApprovalClosesTheTab()
{
    EditorTabs tabs;
    tabs.addEditor();
    edit(tabs.editorAt(0));
    tabs.setCloseGuard([](EditorWidget&) { return true; });

    QVERIFY(tabs.closeTab(0));
    QCOMPARE(tabs.count(), 1);
}

void EditorTabsTest::guardSeesTheTabAlreadyActive()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    edit(tabs.editorAt(0));
    tabs.setCurrentIndex(2);

    EditorWidget* seen = nullptr;
    EditorWidget* current = nullptr;
    tabs.setCloseGuard([&](EditorWidget& editor) {
        seen = &editor;
        current = tabs.currentEditor();
        return true;
    });

    QVERIFY(tabs.closeTab(0)); // closing a background tab must show it to the user first
    QVERIFY(seen != nullptr);
    QCOMPARE(current, seen);
}

void EditorTabsTest::closeOthersDoesNotAskAboutTheKeptTab()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    for (int i = 0; i < 3; ++i) {
        edit(tabs.editorAt(i));
    }
    EditorWidget* keep = tabs.editorAt(1);

    QList<EditorWidget*> asked;
    tabs.setCloseGuard([&asked](EditorWidget& editor) {
        asked.append(&editor);
        return true;
    });

    QVERIFY(tabs.closeOtherTabs(1));
    QCOMPARE(asked.size(), 2);
    QVERIFY(!asked.contains(keep));
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor(), keep);
}

void EditorTabsTest::closeAllStopsAtTheFirstRefusal()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    for (int i = 0; i < 3; ++i) {
        edit(tabs.editorAt(i));
    }
    EditorWidget* second = tabs.editorAt(1);
    EditorWidget* third = tabs.editorAt(2);

    int asked = 0;
    tabs.setCloseGuard([&asked](EditorWidget&) { return ++asked == 1; }); // approve, then refuse

    QVERIFY(!tabs.closeAllTabs());
    QCOMPARE(asked, 2);        // never asked about the third
    QCOMPARE(tabs.count(), 2); // the first was closed before the refusal
    QCOMPARE(tabs.editorAt(0), second);
    QCOMPARE(tabs.editorAt(1), third);
    QCOMPARE(tabs.currentEditor(), second); // the refused tab is the one the user sees
}

void EditorTabsTest::closeAllAsksLeftToRight()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    for (int i = 0; i < 3; ++i) {
        edit(tabs.editorAt(i));
    }
    const QList<EditorWidget*> expected = {tabs.editorAt(0), tabs.editorAt(1), tabs.editorAt(2)};

    QList<EditorWidget*> order;
    tabs.setCloseGuard([&order](EditorWidget& editor) {
        order.append(&editor);
        return true;
    });

    QVERIFY(tabs.closeAllTabs());
    QCOMPARE(order, expected);
    QCOMPARE(tabs.count(), 1);
    QVERIFY(!tabs.currentEditor()->isModified()); // the fresh Untitled tab
}

void EditorTabsTest::tabButtonAndMiddleClickUseTheGuard()
{
    EditorTabs tabs;
    tabs.addEditor();
    edit(tabs.editorAt(0));
    edit(tabs.editorAt(1));
    tabs.resize(600, 300);
    tabs.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabs));

    int asked = 0;
    tabs.setCloseGuard([&asked](EditorWidget&) {
        ++asked;
        return false;
    });

    emit tabs.tabCloseRequested(0);
    QTabBar* bar = tabs.tabBar();
    QTest::mouseClick(bar, Qt::MiddleButton, Qt::NoModifier, bar->tabRect(1).center());

    QCOMPARE(asked, 2);
    QCOMPARE(tabs.count(), 2); // both requests were refused
}

void EditorTabsTest::invalidIndexReturnsFalse()
{
    EditorTabs tabs;
    QVERIFY(!tabs.closeTab(-1));
    QVERIFY(!tabs.closeTab(1));
    QVERIFY(!tabs.closeOtherTabs(3));
}

QTEST_MAIN(EditorTabsTest)

#include "EditorTabsTest.moc"
