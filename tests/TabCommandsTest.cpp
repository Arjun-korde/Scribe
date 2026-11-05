#include "TabCommands.h"

#include "CommandRegistry.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QtTest>

using scribe::core::CommandRegistry;
using scribe::editor::EditorTabs;
using scribe::editor::TabCommands;

class TabCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void registersAllFiveCommands();
    void rejectsBeingRegisteredTwice();
    void shortcutsMatchTheDesign();
    void closeAffectsTheActiveTab();
    void closeOthersKeepsTheActiveTab();
    void closeAllLeavesOneFreshTab();
    void nextAndPreviousCycleWithWrapAround();
};

void TabCommandsTest::registersAllFiveCommands()
{
    EditorTabs tabs;
    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);

    QCOMPARE(registry.commands().size(), size_t{5});
    for (const QLatin1String id :
         {TabCommands::kClose, TabCommands::kCloseOthers, TabCommands::kCloseAll,
          TabCommands::kNext, TabCommands::kPrevious}) {
        const auto* command = registry.find(id);
        QVERIFY2(command != nullptr, qPrintable(id));
        QVERIFY2(!command->title.isEmpty(), qPrintable(id));
    }
}

void TabCommandsTest::rejectsBeingRegisteredTwice()
{
    EditorTabs tabs;
    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);
    TabCommands::registerIn(registry, tabs); // duplicates are ignored by the registry
    QCOMPARE(registry.commands().size(), size_t{5});
}

void TabCommandsTest::shortcutsMatchTheDesign()
{
    EditorTabs tabs;
    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);

    QCOMPARE(registry.find(TabCommands::kClose)->shortcut, QStringLiteral("Ctrl+W"));
    QCOMPARE(registry.find(TabCommands::kCloseAll)->shortcut, QStringLiteral("Ctrl+Shift+W"));
    QCOMPARE(registry.find(TabCommands::kNext)->shortcut, QStringLiteral("Ctrl+Tab"));
    QCOMPARE(registry.find(TabCommands::kPrevious)->shortcut, QStringLiteral("Ctrl+Shift+Tab"));
    QVERIFY(registry.find(TabCommands::kCloseOthers)->shortcut.isEmpty());
}

void TabCommandsTest::closeAffectsTheActiveTab()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    tabs.setCurrentIndex(1);
    auto* first = tabs.editorAt(0);
    auto* last = tabs.editorAt(2);

    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);
    QVERIFY(registry.run(TabCommands::kClose));

    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.editorAt(0), first);
    QCOMPARE(tabs.editorAt(1), last);
}

void TabCommandsTest::closeOthersKeepsTheActiveTab()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    tabs.setCurrentIndex(1);
    auto* keep = tabs.currentEditor();

    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);
    QVERIFY(registry.run(TabCommands::kCloseOthers));

    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.currentEditor(), keep);
}

void TabCommandsTest::closeAllLeavesOneFreshTab()
{
    EditorTabs tabs;
    tabs.currentEditor()->insertPlainText(QStringLiteral("x"));
    tabs.addEditor();

    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);
    QVERIFY(registry.run(TabCommands::kCloseAll));

    QCOMPARE(tabs.count(), 1);
    QVERIFY(!tabs.currentEditor()->isModified());
}

void TabCommandsTest::nextAndPreviousCycleWithWrapAround()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    tabs.setCurrentIndex(2);

    CommandRegistry registry;
    TabCommands::registerIn(registry, tabs);

    QVERIFY(registry.run(TabCommands::kNext));
    QCOMPARE(tabs.currentIndex(), 0);
    QVERIFY(registry.run(TabCommands::kPrevious));
    QCOMPARE(tabs.currentIndex(), 2);
}

QTEST_MAIN(TabCommandsTest)

#include "TabCommandsTest.moc"
