#include "FindBar.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest>

using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;
using scribe::editor::FindBar;

// The find / replace bar: opening, incremental search, navigation, replacing, and its messages.
class FindBarTest : public QObject
{
    Q_OBJECT

private slots:
    void startsClosedWithNothingToFind();
    void openFindShowsTheBarWithoutTheReplaceRow();
    void openReplaceShowsTheReplaceRow();
    void closeBarHidesItKeepsTheQueryAndRemovesTheMark();
    void openFindTakesAOneLineSelectionAsTheQuery();
    void openFindIgnoresMultiLineAndVeryLongSelections();
    void fieldsAndButtonsHaveAccessibleNames();

    void typingSelectsTheFirstMatchFromTheCaret();
    void incrementalSearchStartsFromWhereItBeganNotFromTheLastMatch();
    void noMatchSaysNoResultsAndKeepsTheSelection();
    void matchingIgnoresCaseUnlessAskedNotTo();
    void togglingMatchCaseSearchesAgain();
    void theCurrentMatchIsMarkedForTheEye();
    void theMarkMovesWithTheSelection();

    void statusShowsWhichMatchOfHowMany();
    void statusShowsJustTheCountWhenTheSelectionIsNotAMatch();
    void aSingleMatchReadsAsOneResult();
    void countsAreCappedOnHugeTexts();
    void statusFollowsEditsMadeInTheEditor();

    void findNextMovesToTheNextMatchAndWraps();
    void findPreviousMovesToThePreviousMatchAndWraps();
    void withWrapAroundOffNavigationStopsAtTheEnds();
    void stepsThroughEveryMatchExactlyOnce();
    void findNextWorksWhileTheBarIsClosed();
    void findNextWithNothingToFindOpensTheBar();
    void findNextAfterEditingUsesTheFreshText();
    void movingTheCaretMovesWhereTheNextSearchStarts();
    void switchingTabsSearchesTheNewTab();

    void enterGoesToTheNextMatchAndShiftEnterToThePrevious();
    void escapeClosesTheBarFromEitherField();
    void theButtonsNavigateAndClose();

    void replaceReplacesTheCurrentMatchAndSelectsTheNext();
    void replaceOnANonMatchOnlySelectsTheNextMatch();
    void replaceOfTheLastMatchWrapsOrStops();
    void aReplacementContainingTheQueryDoesNotLoop();
    void enterInTheReplaceFieldReplaces();
    void replaceAllReplacesEverythingAndSaysHowMany();
    void replaceAllIsOneUndoStep();
    void replaceAllWithNothingFoundChangesNothing();
    void replaceAllHonoursMatchCase();
    void replaceAllWithAnEmptyReplacementDeletes();
    void replaceAllUsesTheCurrentTextNotACachedCopy();
    void theReplacedMessageGoesAwayWhenSomethingChanges();
    void replaceButtonsNeedTextToFind();
    void replaceButtonsWork();
    void replaceChangesOnlyTheActiveTab();

private:
    static constexpr const char* kText = "alpha beta alpha gamma alpha";
    // alpha at 0, 11 and 23; beta at 6; gamma at 17.

    struct Fixture
    {
        QWidget host;
        EditorTabs tabs{&host};
        FindBar bar{tabs, &host};

        explicit Fixture(const QString& text = QString::fromLatin1(kText))
        {
            auto* layout = new QVBoxLayout(&host);
            layout->addWidget(&tabs);
            layout->addWidget(&bar);
            host.resize(800, 500);
            editor()->setPlainText(text);
            editor()->moveCursor(QTextCursor::Start);
        }

        [[nodiscard]] EditorWidget* editor() const { return tabs.currentEditor(); }
        [[nodiscard]] int selStart() const { return editor()->textCursor().selectionStart(); }
        [[nodiscard]] QString selected() const { return editor()->textCursor().selectedText(); }
        [[nodiscard]] QLineEdit* field(const QString& name) const
        {
            for (QLineEdit* edit : bar.findChildren<QLineEdit*>()) {
                if (edit->accessibleName() == name) {
                    return edit;
                }
            }
            return nullptr;
        }
        [[nodiscard]] QAbstractButton* button(const QString& name) const
        {
            for (QAbstractButton* button : bar.findChildren<QAbstractButton*>()) {
                if (button->accessibleName() == name || button->text() == name) {
                    return button;
                }
            }
            return nullptr;
        }
        void placeCaret(int position) const
        {
            QTextCursor cursor(editor()->document());
            cursor.setPosition(position);
            editor()->setTextCursor(cursor);
        }
    };
};

// ---- Opening and closing --------------------------------------------------------------------

void FindBarTest::startsClosedWithNothingToFind()
{
    Fixture f;
    QVERIFY(!f.bar.isOpen());
    QVERIFY(f.bar.query().isEmpty());
    QVERIFY(!f.bar.hasQuery());
    QVERIFY(f.bar.wrapAround()); // on by default
    QVERIFY(!f.bar.matchCase());
    QVERIFY(f.bar.statusText().isEmpty());
}

void FindBarTest::openFindShowsTheBarWithoutTheReplaceRow()
{
    Fixture f;
    f.bar.openFind();
    QVERIFY(f.bar.isOpen());
    QVERIFY(!f.bar.isReplaceRowOpen());
}

void FindBarTest::openReplaceShowsTheReplaceRow()
{
    Fixture f;
    f.bar.openReplace();
    QVERIFY(f.bar.isOpen());
    QVERIFY(f.bar.isReplaceRowOpen());

    f.bar.closeBar();
    f.bar.openFind(); // the row stays as the user left it
    QVERIFY(f.bar.isOpen());
}

void FindBarTest::closeBarHidesItKeepsTheQueryAndRemovesTheMark()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("beta"));
    QCOMPARE(f.editor()->extraSelections().size(), 1);

    f.bar.closeBar();

    QVERIFY(!f.bar.isOpen());
    QCOMPARE(f.bar.query(), QStringLiteral("beta")); // F3 can keep using it
    QVERIFY(f.editor()->extraSelections().isEmpty());
    QCOMPARE(f.selected(), QStringLiteral("beta")); // the selection stays
}

void FindBarTest::openFindTakesAOneLineSelectionAsTheQuery()
{
    Fixture f;
    f.placeCaret(6);
    QTextCursor cursor = f.editor()->textCursor();
    cursor.setPosition(10, QTextCursor::KeepAnchor); // "beta"
    f.editor()->setTextCursor(cursor);

    f.bar.openFind();

    QCOMPARE(f.bar.query(), QStringLiteral("beta"));
    QCOMPARE(f.selected(), QStringLiteral("beta"));
}

void FindBarTest::openFindIgnoresMultiLineAndVeryLongSelections()
{
    Fixture f(QStringLiteral("a\nb"));
    f.bar.setQuery(QStringLiteral("old"));
    f.editor()->selectAll(); // spans two lines
    f.bar.openFind();
    QCOMPARE(f.bar.query(), QStringLiteral("old")); // unchanged
    f.bar.closeBar();

    Fixture longText(QString(400, QLatin1Char('x')));
    longText.bar.setQuery(QStringLiteral("keep"));
    longText.editor()->selectAll();
    longText.bar.openFind();
    QCOMPARE(longText.bar.query(), QStringLiteral("keep"));
}

void FindBarTest::fieldsAndButtonsHaveAccessibleNames()
{
    Fixture f;
    QVERIFY(f.field("Find") != nullptr);
    QVERIFY(f.field("Replace with") != nullptr);
    for (const QString& name : {QStringLiteral("Match case"), QStringLiteral("Wrap around"),
                                QStringLiteral("Previous match"), QStringLiteral("Next match"),
                                QStringLiteral("Close")}) {
        QVERIFY2(f.button(name) != nullptr, qPrintable(name));
    }
    QVERIFY(f.bar.findChild<QLabel*>() != nullptr);
    QCOMPARE(f.bar.findChild<QLabel*>()->accessibleName(), QStringLiteral("Search result"));
}

// ---- Incremental search ---------------------------------------------------------------------

void FindBarTest::typingSelectsTheFirstMatchFromTheCaret()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.selStart(), 0);

    f.bar.closeBar();
    f.bar.setQuery(QString());
    f.placeCaret(5);
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.selStart(), 11); // the first one at or after the caret
}

void FindBarTest::incrementalSearchStartsFromWhereItBeganNotFromTheLastMatch()
{
    Fixture f;
    f.placeCaret(6);
    f.bar.openFind();

    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.selStart(), 11);

    // Shortening the text goes back to the first match after the starting point (the "a" in
    // "beta"), instead of staying near the last match.
    f.bar.setQuery(QStringLiteral("a"));
    QCOMPARE(f.selStart(), 9);

    f.bar.setQuery(QStringLiteral("alp"));
    QCOMPARE(f.selStart(), 11);
}

void FindBarTest::noMatchSaysNoResultsAndKeepsTheSelection()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("beta"));
    QCOMPARE(f.selected(), QStringLiteral("beta"));

    f.bar.setQuery(QStringLiteral("zzz"));

    QCOMPARE(f.bar.statusText(), FindBar::noResultsText());
    QCOMPARE(f.selected(), QStringLiteral("beta"));   // nothing was thrown away
    QVERIFY(f.editor()->extraSelections().isEmpty()); // but nothing is marked as a match
}

void FindBarTest::matchingIgnoresCaseUnlessAskedNotTo()
{
    Fixture f(QStringLiteral("Alpha alpha ALPHA"));
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 3"));
}

void FindBarTest::togglingMatchCaseSearchesAgain()
{
    Fixture f(QStringLiteral("Alpha alpha ALPHA"));
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.selStart(), 0);

    f.bar.setMatchCase(true);

    QCOMPARE(f.selStart(), 6); // the only exact match
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 1"));

    f.bar.setMatchCase(false);
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 3"));
}

void FindBarTest::theCurrentMatchIsMarkedForTheEye()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("gamma"));

    const auto marks = f.editor()->extraSelections();
    QCOMPARE(marks.size(), 1);
    QCOMPARE(marks.first().cursor.selectedText(), QStringLiteral("gamma"));
}

void FindBarTest::theMarkMovesWithTheSelection()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.editor()->extraSelections().first().cursor.selectionStart(), 0);

    f.bar.findNext();

    QCOMPARE(f.editor()->extraSelections().size(), 1);
    QCOMPARE(f.editor()->extraSelections().first().cursor.selectionStart(), 11);
}

// ---- Status ---------------------------------------------------------------------------------

void FindBarTest::statusShowsWhichMatchOfHowMany()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 3"));

    f.bar.findNext();
    QCOMPARE(f.bar.statusText(), QStringLiteral("2 of 3"));
    f.bar.findNext();
    QCOMPARE(f.bar.statusText(), QStringLiteral("3 of 3"));
    f.bar.findNext(); // wraps
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 3"));
    f.bar.findPrevious(); // wraps backwards
    QCOMPARE(f.bar.statusText(), QStringLiteral("3 of 3"));
}

void FindBarTest::statusShowsJustTheCountWhenTheSelectionIsNotAMatch()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 3"));

    f.placeCaret(3); // the user clicks somewhere else

    QTRY_COMPARE(f.bar.statusText(), QStringLiteral("3 results"));
}

void FindBarTest::aSingleMatchReadsAsOneResult()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("gamma"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 1"));

    f.placeCaret(0);
    QTRY_COMPARE(f.bar.statusText(), QStringLiteral("1 result"));
}

void FindBarTest::countsAreCappedOnHugeTexts()
{
    QString text;
    text.reserve(2 * (FindBar::kMaxCountedMatches + 10));
    for (int i = 0; i < FindBar::kMaxCountedMatches + 10; ++i) {
        text += QStringLiteral("a ");
    }
    Fixture f(text);
    f.bar.openFind();

    f.bar.setQuery(QStringLiteral("a"));

    QCOMPARE(f.bar.statusText(), FindBar::countText(0, 0, true));
    QVERIFY(f.bar.statusText().contains(QLatin1Char('+')));
    QVERIFY(f.selected() == QStringLiteral("a")); // searching still works
}

void FindBarTest::statusFollowsEditsMadeInTheEditor()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("beta"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 1"));

    f.editor()->moveCursor(QTextCursor::End);
    f.editor()->insertPlainText(QStringLiteral(" beta"));

    QTRY_VERIFY(f.bar.statusText().contains(QStringLiteral("2")));
}

// ---- Navigation -----------------------------------------------------------------------------

void FindBarTest::findNextMovesToTheNextMatchAndWraps()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.selStart(), 0);

    QVERIFY(f.bar.findNext());
    QCOMPARE(f.selStart(), 11);
    QVERIFY(f.bar.findNext());
    QCOMPARE(f.selStart(), 23);
    QVERIFY(f.bar.findNext()); // wraps
    QCOMPARE(f.selStart(), 0);
}

void FindBarTest::findPreviousMovesToThePreviousMatchAndWraps()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QCOMPARE(f.selStart(), 0);

    QVERIFY(f.bar.findPrevious()); // wraps to the last
    QCOMPARE(f.selStart(), 23);
    QVERIFY(f.bar.findPrevious());
    QCOMPARE(f.selStart(), 11);
    QVERIFY(f.bar.findPrevious());
    QCOMPARE(f.selStart(), 0);
}

void FindBarTest::withWrapAroundOffNavigationStopsAtTheEnds()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    f.bar.setWrapAround(false);

    QVERIFY(f.bar.findNext());
    QVERIFY(f.bar.findNext());
    QCOMPARE(f.selStart(), 23);
    QVERIFY(!f.bar.findNext()); // no match after the last one
    QCOMPARE(f.selStart(), 23); // the selection stays on the last match
    QCOMPARE(f.bar.statusText(), QStringLiteral("No more results"));

    QVERIFY(f.bar.findPrevious());
    QVERIFY(f.bar.findPrevious());
    QCOMPARE(f.selStart(), 0);
    QVERIFY(!f.bar.findPrevious());
    QCOMPARE(f.selStart(), 0);
    QCOMPARE(f.bar.statusText(), QStringLiteral("No more results"));
}

void FindBarTest::stepsThroughEveryMatchExactlyOnce()
{
    Fixture f(QStringLiteral("x.x.x.x.x"));
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("x"));

    QList<int> visited{f.selStart()};
    for (int i = 0; i < 4; ++i) {
        QVERIFY(f.bar.findNext());
        visited << f.selStart();
    }
    QCOMPARE(visited, (QList<int>{0, 2, 4, 6, 8}));
    QVERIFY(f.bar.findNext()); // and then it starts over
    QCOMPARE(f.selStart(), 0);
}

void FindBarTest::findNextWorksWhileTheBarIsClosed()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    f.bar.closeBar();

    QVERIFY(f.bar.findNext());

    QCOMPARE(f.selStart(), 11);
    QVERIFY(!f.bar.isOpen()); // finding something does not pop the bar up again
}

void FindBarTest::findNextWithNothingToFindOpensTheBar()
{
    Fixture f;
    QVERIFY(!f.bar.isOpen());

    QVERIFY(!f.bar.findNext());
    QVERIFY(f.bar.isOpen());

    f.bar.closeBar();
    QVERIFY(!f.bar.findPrevious());
    QVERIFY(f.bar.isOpen());
}

void FindBarTest::findNextAfterEditingUsesTheFreshText()
{
    Fixture f(QStringLiteral("one two"));
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("two"));
    f.bar.closeBar();

    f.editor()->moveCursor(QTextCursor::End);
    f.editor()->insertPlainText(QStringLiteral(" two")); // a new match, added right now
    f.placeCaret(0);

    QVERIFY(f.bar.findNext());
    QCOMPARE(f.selStart(), 4);
    QVERIFY(f.bar.findNext());
    QCOMPARE(f.selStart(), 8); // the one just typed
}

void FindBarTest::movingTheCaretMovesWhereTheNextSearchStarts()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha")); // selects the one at 0
    f.placeCaret(20);                        // the user clicks near the end

    QVERIFY(f.bar.findNext());

    QCOMPARE(f.selStart(), 23); // the next one after where the user clicked
}

void FindBarTest::switchingTabsSearchesTheNewTab()
{
    Fixture f(QStringLiteral("x x x"));
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("x"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 of 3"));
    EditorWidget* first = f.editor();

    f.tabs.addEditor();
    f.editor()->setPlainText(QStringLiteral("x"));
    f.editor()->moveCursor(QTextCursor::Start);

    QTRY_COMPARE(f.bar.statusText(), QStringLiteral("1 result"));
    QVERIFY(first->extraSelections().isEmpty()); // the old tab is not left marked

    QVERIFY(f.bar.findNext());
    QCOMPARE(f.editor()->textCursor().selectedText(), QStringLiteral("x"));
    QCOMPARE(f.tabs.editorAt(0)->textCursor().selectionStart(), 0); // the first tab was untouched
}

// ---- Keys and buttons -----------------------------------------------------------------------

void FindBarTest::enterGoesToTheNextMatchAndShiftEnterToThePrevious()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));
    QLineEdit* find = f.field("Find");

    QTest::keyClick(find, Qt::Key_Return);
    QCOMPARE(f.selStart(), 11);
    QTest::keyClick(find, Qt::Key_Enter); // the numeric keypad's Enter
    QCOMPARE(f.selStart(), 23);
    QTest::keyClick(find, Qt::Key_Return, Qt::ShiftModifier);
    QCOMPARE(f.selStart(), 11);
    QTest::keyClick(find, Qt::Key_Return, Qt::ShiftModifier);
    QCOMPARE(f.selStart(), 0);
}

void FindBarTest::escapeClosesTheBarFromEitherField()
{
    Fixture f;
    f.bar.openReplace();
    QTest::keyClick(f.field("Find"), Qt::Key_Escape);
    QVERIFY(!f.bar.isOpen());

    f.bar.openReplace();
    QTest::keyClick(f.field("Replace with"), Qt::Key_Escape);
    QVERIFY(!f.bar.isOpen());
}

void FindBarTest::theButtonsNavigateAndClose()
{
    Fixture f;
    f.bar.openFind();
    f.bar.setQuery(QStringLiteral("alpha"));

    f.button("Next match")->click();
    QCOMPARE(f.selStart(), 11);
    f.button("Previous match")->click();
    QCOMPARE(f.selStart(), 0);
    f.button("Wrap around")->click(); // toggles
    QVERIFY(!f.bar.wrapAround());
    f.button("Match case")->click();
    QVERIFY(f.bar.matchCase());

    f.button("Close")->click();
    QVERIFY(!f.bar.isOpen());
}

// ---- Replace --------------------------------------------------------------------------------

void FindBarTest::replaceReplacesTheCurrentMatchAndSelectsTheNext()
{
    Fixture f(QStringLiteral("cat cat cat"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat")); // selects the first
    f.bar.setReplacement(QStringLiteral("dog"));

    QVERIFY(f.bar.replaceCurrent());
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog cat cat"));
    QCOMPARE(f.selStart(), 4); // the next match is selected, ready for the next click

    QVERIFY(f.bar.replaceCurrent());
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog dog cat"));
    QVERIFY(f.bar.replaceCurrent());
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog dog dog"));
    QCOMPARE(f.bar.statusText(), FindBar::noResultsText()); // nothing left to replace
}

void FindBarTest::replaceOnANonMatchOnlySelectsTheNextMatch()
{
    Fixture f(QStringLiteral("cat cat cat"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));
    f.placeCaret(5); // inside the second "cat": the selection is not a match

    QVERIFY(!f.bar.replaceCurrent()); // nothing replaced...

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("cat cat cat"));
    QCOMPARE(f.selStart(), 8); // ...it only moved to the next match
}

void FindBarTest::replaceOfTheLastMatchWrapsOrStops()
{
    {
        Fixture f(QStringLiteral("a-b-a"));
        f.bar.openReplace();
        f.bar.setQuery(QStringLiteral("a"));
        f.bar.setReplacement(QStringLiteral("X"));
        f.bar.findNext(); // the last "a"
        QCOMPARE(f.selStart(), 4);

        QVERIFY(f.bar.replaceCurrent());

        QCOMPARE(f.editor()->toPlainText(), QStringLiteral("a-b-X"));
        QCOMPARE(f.selStart(), 0); // wrapped to the first
    }
    {
        Fixture f(QStringLiteral("a-b-a"));
        f.bar.openReplace();
        f.bar.setQuery(QStringLiteral("a"));
        f.bar.setReplacement(QStringLiteral("X"));
        f.bar.setWrapAround(false);
        f.bar.findNext();
        QCOMPARE(f.selStart(), 4);

        QVERIFY(f.bar.replaceCurrent());

        QCOMPARE(f.editor()->toPlainText(), QStringLiteral("a-b-X"));
        QCOMPARE(f.bar.statusText(), QStringLiteral("No more results"));
    }
}

void FindBarTest::aReplacementContainingTheQueryDoesNotLoop()
{
    Fixture f(QStringLiteral("a a a"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("a"));
    f.bar.setReplacement(QStringLiteral("aa"));

    QVERIFY(f.bar.replaceCurrent());

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("aa a a"));
    QCOMPARE(f.selStart(), 3); // the next original "a", not inside the text just inserted

    // "aa a a" now holds four separate a's (the inserted "aa" has two), and each becomes "aa".
    QCOMPARE(f.bar.replaceAll(), 4);
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("aaaa aa aa"));
}

void FindBarTest::enterInTheReplaceFieldReplaces()
{
    Fixture f(QStringLiteral("cat cat"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));

    QTest::keyClick(f.field("Replace with"), Qt::Key_Return);

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog cat"));
}

void FindBarTest::replaceAllReplacesEverythingAndSaysHowMany()
{
    Fixture f(QStringLiteral("cat cat cat"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));

    QCOMPARE(f.bar.replaceAll(), 3);

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog dog dog"));
    QCOMPARE(f.bar.statusText(), FindBar::replacedText(3));
    QCOMPARE(f.bar.statusText(), QStringLiteral("3 replaced"));
    QVERIFY(f.editor()->isModified());
}

void FindBarTest::replaceAllIsOneUndoStep()
{
    Fixture f(QStringLiteral("cat cat cat"));
    f.editor()->document()->clearUndoRedoStacks();
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));
    f.bar.replaceAll();

    f.editor()->undo(); // one Ctrl+Z

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("cat cat cat"));
    QVERIFY(!f.editor()->document()->isUndoAvailable());
}

void FindBarTest::replaceAllWithNothingFoundChangesNothing()
{
    Fixture f(QStringLiteral("abc"));
    f.editor()->document()->setModified(false);
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("zzz"));
    f.bar.setReplacement(QStringLiteral("x"));

    QCOMPARE(f.bar.replaceAll(), 0);

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("abc"));
    QCOMPARE(f.bar.statusText(), FindBar::noResultsText());
    QVERIFY(!f.editor()->isModified());
}

void FindBarTest::replaceAllHonoursMatchCase()
{
    Fixture f(QStringLiteral("Cat cat CAT"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));
    f.bar.setMatchCase(true);

    QCOMPARE(f.bar.replaceAll(), 1);
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("Cat dog CAT"));
}

void FindBarTest::replaceAllWithAnEmptyReplacementDeletes()
{
    Fixture f(QStringLiteral("a-b-c"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("-"));
    f.bar.setReplacement(QString());

    QCOMPARE(f.bar.replaceAll(), 2);
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("abc"));
}

void FindBarTest::replaceAllUsesTheCurrentTextNotACachedCopy()
{
    Fixture f(QStringLiteral("x"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("x"));
    f.bar.setReplacement(QStringLiteral("y"));

    // Edit and replace at once, before any timer has had a chance to refresh anything.
    f.editor()->moveCursor(QTextCursor::End);
    f.editor()->insertPlainText(QStringLiteral(" x x"));
    QCOMPARE(f.bar.replaceAll(), 3);

    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("y y y"));
}

void FindBarTest::theReplacedMessageGoesAwayWhenSomethingChanges()
{
    const auto replacedBar = [](Fixture& f) {
        f.bar.openReplace();
        f.bar.setQuery(QStringLiteral("cat"));
        f.bar.setReplacement(QStringLiteral("dog"));
        f.bar.replaceAll();
        QCOMPARE(f.bar.statusText(), QStringLiteral("3 replaced"));
    };

    { // typing a new query
        Fixture f(QStringLiteral("cat cat cat"));
        replacedBar(f);
        f.bar.setQuery(QStringLiteral("dog"));
        // A new search starts at the caret, which is after the first replacement.
        QCOMPARE(f.bar.statusText(), QStringLiteral("2 of 3"));
    }
    { // going to another match
        Fixture f(QStringLiteral("cat cat cat"));
        replacedBar(f);
        f.bar.setQuery(QStringLiteral("dog"));
        f.bar.findNext();
        QCOMPARE(f.bar.statusText(), QStringLiteral("3 of 3"));
    }
    { // editing the text
        Fixture f(QStringLiteral("cat cat cat"));
        replacedBar(f);
        f.editor()->moveCursor(QTextCursor::End);
        f.editor()->insertPlainText(QStringLiteral("!"));
        QTRY_VERIFY(f.bar.statusText() != QStringLiteral("3 replaced"));
    }
    { // undoing
        Fixture f(QStringLiteral("cat cat cat"));
        f.editor()->document()->clearUndoRedoStacks();
        replacedBar(f);
        f.editor()->undo();
        QTRY_VERIFY(f.bar.statusText() != QStringLiteral("3 replaced"));
    }
}

void FindBarTest::replaceButtonsNeedTextToFind()
{
    Fixture f;
    QVERIFY(!f.button("Replace")->isEnabled());
    QVERIFY(!f.button("Replace All")->isEnabled());

    f.bar.setQuery(QStringLiteral("x"));
    QVERIFY(f.button("Replace")->isEnabled());
    QVERIFY(f.button("Replace All")->isEnabled());

    f.bar.setQuery(QString());
    QVERIFY(!f.button("Replace")->isEnabled());
}

void FindBarTest::replaceButtonsWork()
{
    Fixture f(QStringLiteral("cat cat"));
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));

    f.button("Replace")->click();
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog cat"));

    f.button("Replace All")->click();
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("dog dog"));
    QCOMPARE(f.bar.statusText(), QStringLiteral("1 replaced"));
}

void FindBarTest::replaceChangesOnlyTheActiveTab()
{
    Fixture f(QStringLiteral("cat"));
    f.tabs.addEditor();
    f.editor()->setPlainText(QStringLiteral("cat cat"));
    f.editor()->moveCursor(QTextCursor::Start);
    f.bar.openReplace();
    f.bar.setQuery(QStringLiteral("cat"));
    f.bar.setReplacement(QStringLiteral("dog"));

    QCOMPARE(f.bar.replaceAll(), 2);

    QCOMPARE(f.tabs.editorAt(1)->toPlainText(), QStringLiteral("dog dog"));
    QCOMPARE(f.tabs.editorAt(0)->toPlainText(), QStringLiteral("cat")); // the other tab is intact
}

QTEST_MAIN(FindBarTest)

#include "FindBarTest.moc"
