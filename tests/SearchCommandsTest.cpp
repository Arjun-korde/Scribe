#include "SearchCommands.h"

#include "CommandRegistry.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QtTest>

using scribe::core::CommandRegistry;
using scribe::editor::EditorTabs;
using scribe::editor::SearchCommands;

class SearchCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void registersGoToLineWithCtrlG();
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
            SearchCommands::registerIn(registry, tabs, chooser);
        }

        bool run() const { return registry.run(SearchCommands::kGoToLine); }
    };
};

void SearchCommandsTest::registersGoToLineWithCtrlG()
{
    Fixture f;
    QCOMPARE(f.registry.commands().size(), size_t{1});

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

QTEST_MAIN(SearchCommandsTest)

#include "SearchCommandsTest.moc"
