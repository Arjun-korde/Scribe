#include "SearchCommands.h"

#include "CommandRegistry.h"
#include "EditorTabs.h"
#include "EditorWidget.h"
#include "FindBar.h"

#include <QtTest>

using scribe::core::CommandRegistry;
using scribe::editor::EditorTabs;
using scribe::editor::FindBar;
using scribe::editor::SearchCommands;

class SearchCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void registersGoToLineWithCtrlG();
    void registersTheFindCommandsWithTheirShortcuts();
    void replaceCommandsNeedTextToFind();
    void findCommandsOpenAndUseTheBar();
    void theChooserReceivesTheCurrentLineAndTheLineCount();
    void theChosenLineGetsTheCaret();
    void cancellingLeavesTheCaretAlone();
    void aChoiceOutsideTheDocumentIsIgnored();
    void isUnavailableWithoutADialog();
    void actsOnTheActiveTab();

private:
    // Tabs with a registry whose "dialog" is a function the test controls.
    struct Fixture
    {
        EditorTabs tabs;
        FindBar findBar{tabs};
        CommandRegistry registry;
        std::optional<int> answer;
        int asked = 0;
        int askedCurrent = 0;
        int askedCount = 0;

        explicit Fixture(bool withDialog = true)
        {
            SearchCommands::LineNumberChooser chooser;
            if (withDialog) {
                chooser = [this](int currentLine, int lineCount) {
                    ++asked;
                    askedCurrent = currentLine;
                    askedCount = lineCount;
                    return answer;
                };
            }
            SearchCommands::registerIn(registry, tabs, findBar, chooser);
        }

        bool run() const { return registry.run(SearchCommands::kGoToLine); }
    };
};

void SearchCommandsTest::registersGoToLineWithCtrlG()
{
    Fixture f;
    QCOMPARE(f.registry.commands().size(), size_t{7}); // go to line plus the find commands

    const auto* command = f.registry.find(SearchCommands::kGoToLine);
    QVERIFY(command != nullptr);
    QCOMPARE(command->shortcut, QStringLiteral("Ctrl+G"));
    QVERIFY(!command->title.isEmpty());
}

void SearchCommandsTest::theChooserReceivesTheCurrentLineAndTheLineCount()
{
    Fixture f;
    f.tabs.currentEditor()->setPlainText(QStringLiteral("a\nb\nc\nd\ne"));
    f.tabs.currentEditor()->goToLine(4);

    QVERIFY(f.run());

    QCOMPARE(f.asked, 1);
    QCOMPARE(f.askedCurrent, 4); // the dialog opens on the line the caret is on
    QCOMPARE(f.askedCount, 5);   // and knows how far it may go
}

void SearchCommandsTest::theChosenLineGetsTheCaret()
{
    Fixture f;
    f.tabs.currentEditor()->setPlainText(QStringLiteral("a\nb\nc\nd"));
    f.answer = 3;

    QVERIFY(f.run());

    QCOMPARE(f.tabs.currentEditor()->currentLine(), 3);
    QCOMPARE(f.tabs.currentEditor()->textCursor().positionInBlock(), 0);
}

void SearchCommandsTest::cancellingLeavesTheCaretAlone()
{
    Fixture f;
    f.tabs.currentEditor()->setPlainText(QStringLiteral("a\nb\nc"));
    f.tabs.currentEditor()->goToLine(2);
    f.answer = std::nullopt; // the user pressed Cancel

    QVERIFY(f.run()); // the command ran; the user just did not choose a line

    QCOMPARE(f.asked, 1);
    QCOMPARE(f.tabs.currentEditor()->currentLine(), 2);
}

void SearchCommandsTest::aChoiceOutsideTheDocumentIsIgnored()
{
    // The real dialog never returns one, but a faulty chooser must not corrupt the caret.
    Fixture f;
    f.tabs.currentEditor()->setPlainText(QStringLiteral("a\nb\nc"));
    f.tabs.currentEditor()->goToLine(2);
    f.answer = 99;

    QVERIFY(f.run());

    QCOMPARE(f.tabs.currentEditor()->currentLine(), 2);
}

void SearchCommandsTest::isUnavailableWithoutADialog()
{
    Fixture f(/*withDialog=*/false);
    QVERIFY(!f.registry.isEnabled(SearchCommands::kGoToLine));
    QVERIFY(!f.run());
}

void SearchCommandsTest::actsOnTheActiveTab()
{
    Fixture f;
    f.tabs.currentEditor()->setPlainText(QStringLiteral("1\n2\n3"));
    f.tabs.addEditor();
    f.tabs.currentEditor()->setPlainText(QStringLiteral("a\nb\nc\nd\ne\nf\ng"));
    f.answer = 6;

    QVERIFY(f.run());

    QCOMPARE(f.askedCount, 7); // the second tab's lines, not the first tab's
    QCOMPARE(f.tabs.editorAt(1)->currentLine(), 6);
    QCOMPARE(f.tabs.editorAt(0)->currentLine(), 1);
}

void SearchCommandsTest::registersTheFindCommandsWithTheirShortcuts()
{
    Fixture f;
    const auto shortcut = [&](QLatin1String id) { return f.registry.find(id)->shortcut; };

    QCOMPARE(shortcut(SearchCommands::kFind), QStringLiteral("Ctrl+F"));
    QCOMPARE(shortcut(SearchCommands::kFindNext), QStringLiteral("F3"));
    QCOMPARE(shortcut(SearchCommands::kFindPrevious), QStringLiteral("Shift+F3"));
    QCOMPARE(shortcut(SearchCommands::kReplace), QStringLiteral("Ctrl+H"));
    QVERIFY(shortcut(SearchCommands::kReplaceNext).isEmpty());
    QVERIFY(shortcut(SearchCommands::kReplaceAll).isEmpty());
}

void SearchCommandsTest::replaceCommandsNeedTextToFind()
{
    Fixture f;
    QVERIFY(!f.registry.isEnabled(SearchCommands::kReplaceNext));
    QVERIFY(!f.registry.isEnabled(SearchCommands::kReplaceAll));
    QVERIFY(!f.registry.run(SearchCommands::kReplaceAll));

    f.findBar.setQuery(QStringLiteral("x"));
    QVERIFY(f.registry.isEnabled(SearchCommands::kReplaceNext));
    QVERIFY(f.registry.isEnabled(SearchCommands::kReplaceAll));

    // The find commands themselves are always available (they open the bar when needed).
    f.findBar.setQuery(QString());
    QVERIFY(f.registry.isEnabled(SearchCommands::kFind));
    QVERIFY(f.registry.isEnabled(SearchCommands::kFindNext));
    QVERIFY(f.registry.isEnabled(SearchCommands::kFindPrevious));
    QVERIFY(f.registry.isEnabled(SearchCommands::kReplace));
}

void SearchCommandsTest::findCommandsOpenAndUseTheBar()
{
    Fixture f;
    f.tabs.currentEditor()->setPlainText(QStringLiteral("one two one two"));
    f.tabs.currentEditor()->moveCursor(QTextCursor::Start);
    QVERIFY(!f.findBar.isOpen());

    QVERIFY(f.registry.run(SearchCommands::kFind));
    QVERIFY(f.findBar.isOpen());
    QVERIFY(!f.findBar.isReplaceRowOpen());

    QVERIFY(f.registry.run(SearchCommands::kReplace));
    QVERIFY(f.findBar.isReplaceRowOpen());

    f.findBar.setQuery(QStringLiteral("two"));
    QCOMPARE(f.tabs.currentEditor()->textCursor().selectedText(), QStringLiteral("two"));
    QVERIFY(f.registry.run(SearchCommands::kFindNext));
    QCOMPARE(f.tabs.currentEditor()->textCursor().selectionStart(), 12);
    QVERIFY(f.registry.run(SearchCommands::kFindPrevious));
    QCOMPARE(f.tabs.currentEditor()->textCursor().selectionStart(), 4);
}

QTEST_MAIN(SearchCommandsTest)

#include "SearchCommandsTest.moc"
