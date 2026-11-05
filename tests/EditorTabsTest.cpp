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

QTEST_MAIN(EditorTabsTest)

#include "EditorTabsTest.moc"
