#include "EditorTabBar.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QSignalSpy>
#include <QToolButton>
#include <QtTest>

using scribe::editor::EditorTabBar;
using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;

/// The parts of the main window that follow the Figma design: the tab strip and the line numbers.
class TabStripTest : public QObject
{
    Q_OBJECT

private slots:
    void tabsAreFixedWidthAndTheFirstHasTheLeadIn()
    {
        EditorTabs tabs;
        tabs.addEditor();
        tabs.addEditor();
        tabs.resize(900, 300);
        tabs.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tabs));

        const QTabBar* bar = tabs.tabBar();
        const int tab = EditorTabBar::kTabWidth + EditorTabBar::kTabSpacing;
        QCOMPARE(bar->tabRect(0).width(), EditorTabBar::kLeadIn + tab);
        QCOMPARE(bar->tabRect(1).width(), tab);
        QCOMPARE(bar->tabRect(2).width(), tab);
        QCOMPARE(bar->tabRect(1).height(), EditorTabBar::kTopGap + EditorTabBar::kTabHeight);
    }

    void closeButtonRequestsTheCloseOfItsOwnTab()
    {
        EditorTabs tabs;
        tabs.addEditor();
        tabs.addEditor();
        tabs.resize(900, 300);
        tabs.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tabs));

        QSignalSpy spy(&tabs, &QTabWidget::tabCloseRequested);
        auto* close = qobject_cast<QToolButton*>(tabs.tabBar()->tabButton(1, QTabBar::RightSide));
        QVERIFY(close != nullptr);
        close->click();

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toInt(), 1);
        QCOMPARE(tabs.count(), 2); // three tabs, and the request closed one
    }

    void everyTabHasACloseButton()
    {
        EditorTabs tabs;
        tabs.addEditor();
        for (int i = 0; i < tabs.count(); ++i) {
            QVERIFY(tabs.tabBar()->tabButton(i, QTabBar::RightSide) != nullptr);
        }
    }

    void newTabButtonAddsATabAndMakesItActive()
    {
        EditorTabs tabs;
        tabs.resize(900, 300);
        tabs.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tabs));
        QCOMPARE(tabs.count(), 1);

        auto* button = tabs.tabBar()->findChild<QToolButton*>("newTabButton");
        QVERIFY(button != nullptr);
        button->click();

        QCOMPARE(tabs.count(), 2);
        QCOMPARE(tabs.currentIndex(), 1);
    }

    void newTabButtonSitsRightAfterTheLastTab()
    {
        EditorTabs tabs;
        tabs.addEditor();
        tabs.resize(900, 300);
        tabs.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tabs));

        auto* button = tabs.tabBar()->findChild<QToolButton*>("newTabButton");
        QVERIFY(button != nullptr);
        const int lastRight = tabs.tabBar()->tabRect(tabs.count() - 1).right();
        QTRY_COMPARE(button->x(), lastRight + 1);

        tabs.addEditor();
        const int newLastRight = tabs.tabBar()->tabRect(tabs.count() - 1).right();
        QVERIFY(newLastRight > lastRight);
        QTRY_COMPARE(button->x(), newLastRight + 1);
    }

    void gutterReservesRoomForTheLineNumbers()
    {
        EditorWidget editor;
        editor.resize(600, 300);
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));

        QVERIFY(editor.viewport()->x() >= 60); // the design's gutter, 56 px plus the text margin
    }

    void gutterGrowsForVeryLongDocuments()
    {
        EditorWidget editor;
        editor.resize(600, 300);
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));
        const int narrow = editor.viewport()->x();

        editor.setPlainText(QString("x\n").repeated(2000));
        const int medium = editor.viewport()->x();
        QVERIFY(medium >= narrow);

        editor.setPlainText(QString("x\n").repeated(100'000));
        QVERIFY(editor.viewport()->x() >= medium);
    }

    void aNewEditorIsCleanAndHasNothingToUndo()
    {
        // The text margin is set in the constructor; it must not count as an edit.
        EditorWidget editor;
        QVERIFY(!editor.isModified());
        QVERIFY(!editor.document()->isUndoAvailable());
    }
};

QTEST_MAIN(TabStripTest)
#include "TabStripTest.moc"
