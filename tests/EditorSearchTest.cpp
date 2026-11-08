#include "EditorSearch.h"

#include "EditorWidget.h"
#include "TextSearch.h"

#include <QScrollBar>
#include <QTextBlock>
#include <QtTest>

using scribe::core::replaceAll;
using scribe::core::SearchOptions;
using scribe::core::TextRange;
using scribe::editor::EditorSearch;
using scribe::editor::EditorWidget;

class EditorSearchTest : public QObject
{
    Q_OBJECT

private slots:
    void positionsOfPlainTextAreTheDocumentsPositions();
    void selectSelectsTheRange();
    void selectScrollsToAFarMatchAndCentersIt();
    void selectDoesNotScrollForAVisibleMatch();

    void selectionIsMatchComparesTheSelectedText();
    void selectionIsMatchHonoursMatchCase();
    void selectionIsMatchNeedsASelectionAndANeedle();
    void selectionIsMatchSeesLineBreaks();

    void replaceSelectionReplacesJustTheSelection();
    void replaceSelectionIsItsOwnUndoStep();
    void replaceSelectionWithoutASelectionDoesNothing();

    void replaceRangesReplacesEveryRange();
    void replaceRangesWorksWhenTheReplacementIsLonger();
    void replaceRangesWorksWhenTheReplacementIsShorter();
    void replaceRangesWithAnEmptyReplacementDeletes();
    void replaceRangesIsOneUndoStepForAllOfThem();
    void undoAfterReplaceRangesRestoresTheExactText();
    void replaceRangesKeepsTheUndoHistoryThatWasThereBefore();
    void replaceRangesMatchesTheCoreReplaceAll();
    void replaceRangesLeavesTheCaretAfterTheFirstReplacement();
    void replaceRangesWithNoRangesChangesNothing();
    void replaceRangesHandlesManyRanges();
    void replaceRangesAcrossLinesKeepsTheLineBreaks();
    void replaceRangesMarksTheDocumentModified();

    void highlightMarksTheMatchEvenWithoutFocus();
    void highlightNulloptRemovesTheMark();

private:
    static EditorWidget* editorWith(const QString& text)
    {
        auto* editor = new EditorWidget;
        editor->setPlainText(text);
        editor->document()->setModified(false);
        return editor;
    }
    static QString numbered(int count)
    {
        QStringList lines;
        for (int i = 1; i <= count; ++i) {
            lines << QStringLiteral("line %1").arg(i);
        }
        return lines.join(QLatin1Char('\n'));
    }
};

void EditorSearchTest::positionsOfPlainTextAreTheDocumentsPositions()
{
    // The search runs on toPlainText() and the editor edits by QTextDocument position, so the two
    // numberings must agree, including across line breaks and emoji (two UTF-16 code units).
    const std::unique_ptr<EditorWidget> editor(
        editorWith(QString::fromUtf8("ab\n\xF0\x9F\x98\x80x\nlast")));
    const QString plain = editor->toPlainText();

    for (const QString& needle :
         {QStringLiteral("last"), QStringLiteral("x"), QStringLiteral("b")}) {
        const qsizetype position = plain.indexOf(needle);
        QTextCursor cursor(editor->document());
        cursor.setPosition(static_cast<int>(position));
        cursor.setPosition(static_cast<int>(position + needle.size()), QTextCursor::KeepAnchor);
        QCOMPARE(cursor.selectedText(), needle);
    }
}

void EditorSearchTest::selectSelectsTheRange()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("one two three")));

    EditorSearch::select(*editor, TextRange{4, 3});

    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("two"));
    QCOMPARE(editor->textCursor().selectionStart(), 4);
    QCOMPARE(editor->textCursor().selectionEnd(), 7);
}

void EditorSearchTest::selectScrollsToAFarMatchAndCentersIt()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(numbered(1000)));
    editor->resize(400, 240);
    editor->show();
    QVERIFY(QTest::qWaitForWindowExposed(editor.get()));
    QCOMPARE(editor->verticalScrollBar()->value(), 0);

    const qsizetype position = editor->toPlainText().indexOf(QStringLiteral("line 700"));
    EditorSearch::select(*editor, TextRange{position, 8});

    const QRect caret = editor->cursorRect();
    QVERIFY(editor->viewport()->rect().contains(caret));
    const int lineHeight = editor->fontMetrics().lineSpacing();
    QVERIFY(qAbs(caret.center().y() - editor->viewport()->height() / 2) <= 2 * lineHeight);
}

void EditorSearchTest::selectDoesNotScrollForAVisibleMatch()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(numbered(1000)));
    editor->resize(400, 400);
    editor->show();
    QVERIFY(QTest::qWaitForWindowExposed(editor.get()));

    EditorSearch::select(*editor, TextRange{0, 4}); // "line", already on screen

    QCOMPARE(editor->verticalScrollBar()->value(), 0);
}

void EditorSearchTest::selectionIsMatchComparesTheSelectedText()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("one two")));
    SearchOptions options;
    EditorSearch::select(*editor, TextRange{4, 3});

    QVERIFY(EditorSearch::selectionIsMatch(*editor, QStringLiteral("two"), options));
    QVERIFY(!EditorSearch::selectionIsMatch(*editor, QStringLiteral("one"), options));
    QVERIFY(!EditorSearch::selectionIsMatch(*editor, QStringLiteral("tw"), options)); // not exact
    QVERIFY(!EditorSearch::selectionIsMatch(*editor, QStringLiteral("twos"), options));
}

void EditorSearchTest::selectionIsMatchHonoursMatchCase()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("Hello")));
    EditorSearch::select(*editor, TextRange{0, 5});

    SearchOptions ignoreCase;
    ignoreCase.matchCase = false;
    SearchOptions matchCase;
    matchCase.matchCase = true;

    QVERIFY(EditorSearch::selectionIsMatch(*editor, QStringLiteral("hello"), ignoreCase));
    QVERIFY(!EditorSearch::selectionIsMatch(*editor, QStringLiteral("hello"), matchCase));
    QVERIFY(EditorSearch::selectionIsMatch(*editor, QStringLiteral("Hello"), matchCase));
}

void EditorSearchTest::selectionIsMatchNeedsASelectionAndANeedle()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("abc")));
    SearchOptions options;

    QVERIFY(!EditorSearch::selectionIsMatch(*editor, QStringLiteral("a"), options)); // no selection
    EditorSearch::select(*editor, TextRange{0, 1});
    QVERIFY(!EditorSearch::selectionIsMatch(*editor, QString(), options)); // empty needle
}

void EditorSearchTest::selectionIsMatchSeesLineBreaks()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a\nb")));
    EditorSearch::select(*editor, TextRange{0, 3});

    // Qt reports a line break in a selection as U+2029; the comparison must use "\n".
    QVERIFY(EditorSearch::selectionIsMatch(*editor, QStringLiteral("a\nb"), SearchOptions{}));
}

void EditorSearchTest::replaceSelectionReplacesJustTheSelection()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("one two three")));
    EditorSearch::select(*editor, TextRange{4, 3});

    EditorSearch::replaceSelection(*editor, QStringLiteral("2"));

    QCOMPARE(editor->toPlainText(), QStringLiteral("one 2 three"));
    QCOMPARE(editor->textCursor().position(), 5); // after the replacement
    QVERIFY(!editor->textCursor().hasSelection());
}

void EditorSearchTest::replaceSelectionIsItsOwnUndoStep()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QString()));
    editor->insertPlainText(QStringLiteral("hello world"));
    EditorSearch::select(*editor, TextRange{6, 5});

    EditorSearch::replaceSelection(*editor, QStringLiteral("there"));
    QCOMPARE(editor->toPlainText(), QStringLiteral("hello there"));

    editor->undo(); // undoes the replacement only, not the typing before it
    QCOMPARE(editor->toPlainText(), QStringLiteral("hello world"));
}

void EditorSearchTest::replaceSelectionWithoutASelectionDoesNothing()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("abc")));
    EditorSearch::replaceSelection(*editor, QStringLiteral("X"));
    QCOMPARE(editor->toPlainText(), QStringLiteral("abc"));
}

void EditorSearchTest::replaceRangesReplacesEveryRange()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a-b-a-b-a")));
    const auto ranges = QList<TextRange>{{0, 1}, {4, 1}, {8, 1}};

    QCOMPARE(EditorSearch::replaceRanges(*editor, ranges, QStringLiteral("X")), 3);

    QCOMPARE(editor->toPlainText(), QStringLiteral("X-b-X-b-X"));
}

void EditorSearchTest::replaceRangesWorksWhenTheReplacementIsLonger()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a.b.c")));
    EditorSearch::replaceRanges(*editor, {{1, 1}, {3, 1}}, QStringLiteral("<<<"));
    QCOMPARE(editor->toPlainText(), QStringLiteral("a<<<b<<<c"));
}

void EditorSearchTest::replaceRangesWorksWhenTheReplacementIsShorter()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a---b---c")));
    EditorSearch::replaceRanges(*editor, {{1, 3}, {5, 3}}, QStringLiteral("-"));
    QCOMPARE(editor->toPlainText(), QStringLiteral("a-b-c"));
}

void EditorSearchTest::replaceRangesWithAnEmptyReplacementDeletes()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a-b-c")));
    EditorSearch::replaceRanges(*editor, {{1, 1}, {3, 1}}, QString());
    QCOMPARE(editor->toPlainText(), QStringLiteral("abc"));
}

void EditorSearchTest::replaceRangesIsOneUndoStepForAllOfThem()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("x x x x x x x x")));
    QList<TextRange> ranges;
    for (int i = 0; i < 8; ++i) {
        ranges.append({qsizetype{i} * 2, 1});
    }

    EditorSearch::replaceRanges(*editor, ranges, QStringLiteral("yy"));
    QCOMPARE(editor->toPlainText(), QStringLiteral("yy yy yy yy yy yy yy yy"));

    editor->undo(); // ONE Ctrl+Z must undo all eight
    QCOMPARE(editor->toPlainText(), QStringLiteral("x x x x x x x x"));
    QVERIFY(!editor->document()->isUndoAvailable()); // nothing else was in the history
}

void EditorSearchTest::undoAfterReplaceRangesRestoresTheExactText()
{
    const QString original = QStringLiteral("The cat sat.\nThe cat ran.\n\nCat!");
    const std::unique_ptr<EditorWidget> editor(editorWith(original));
    const auto ranges = scribe::core::findAll(original, QStringLiteral("cat"), SearchOptions{});

    EditorSearch::replaceRanges(*editor, ranges, QStringLiteral("elephant"));
    QCOMPARE(editor->toPlainText(),
             QStringLiteral("The elephant sat.\nThe elephant ran.\n\nelephant!"));

    editor->undo();
    QCOMPARE(editor->toPlainText(), original);

    editor->redo();
    QCOMPARE(editor->toPlainText(),
             QStringLiteral("The elephant sat.\nThe elephant ran.\n\nelephant!"));
}

void EditorSearchTest::replaceRangesKeepsTheUndoHistoryThatWasThereBefore()
{
    // Replacing must not wipe the user's earlier undo steps (setPlainText() would).
    const std::unique_ptr<EditorWidget> editor(editorWith(QString()));
    editor->insertPlainText(QStringLiteral("typed text"));
    QVERIFY(editor->document()->isUndoAvailable());

    EditorSearch::replaceRanges(*editor, {{0, 5}}, QStringLiteral("TYPED"));
    QCOMPARE(editor->toPlainText(), QStringLiteral("TYPED text"));

    editor->undo(); // the replacement
    QCOMPARE(editor->toPlainText(), QStringLiteral("typed text"));
    QVERIFY(editor->document()->isUndoAvailable()); // the typing can still be undone
    editor->undo();
    QCOMPARE(editor->toPlainText(), QString());
}

void EditorSearchTest::replaceRangesMatchesTheCoreReplaceAll()
{
    // The cursor-based edit must give exactly what the pure function computes.
    const QString text = QStringLiteral("aXbXXc\nXdX\n\nXXXX e X");
    for (const QString& replacement : {QString(), QStringLiteral("-"), QStringLiteral("+++"),
                                       QStringLiteral("X"), QStringLiteral("a\nb")}) {
        const std::unique_ptr<EditorWidget> editor(editorWith(text));
        const SearchOptions options;
        const auto ranges = scribe::core::findAll(text, QStringLiteral("X"), options);

        EditorSearch::replaceRanges(*editor, ranges, replacement);

        QCOMPARE(editor->toPlainText(),
                 replaceAll(text, QStringLiteral("X"), replacement, options).text);
    }
}

void EditorSearchTest::replaceRangesLeavesTheCaretAfterTheFirstReplacement()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("..ab..ab..")));
    EditorSearch::replaceRanges(*editor, {{2, 2}, {6, 2}}, QStringLiteral("XYZ"));

    QCOMPARE(editor->toPlainText(), QStringLiteral("..XYZ..XYZ.."));
    QCOMPARE(editor->textCursor().position(), 5); // right after the first "XYZ"
    QVERIFY(!editor->textCursor().hasSelection());
}

void EditorSearchTest::replaceRangesWithNoRangesChangesNothing()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("abc")));
    QCOMPARE(EditorSearch::replaceRanges(*editor, {}, QStringLiteral("X")), 0);
    QCOMPARE(editor->toPlainText(), QStringLiteral("abc"));
    QVERIFY(!editor->document()->isUndoAvailable());
}

void EditorSearchTest::replaceRangesHandlesManyRanges()
{
    QString text;
    for (int i = 0; i < 5000; ++i) {
        text += QStringLiteral("word ");
    }
    const std::unique_ptr<EditorWidget> editor(editorWith(text));
    const auto ranges = scribe::core::findAll(text, QStringLiteral("word"), SearchOptions{});
    QCOMPARE(ranges.size(), 5000);

    QCOMPARE(EditorSearch::replaceRanges(*editor, ranges, QStringLiteral("w")), 5000);

    QCOMPARE(editor->toPlainText(),
             replaceAll(text, QStringLiteral("word"), QStringLiteral("w"), SearchOptions{}).text);
    editor->undo();
    QCOMPARE(editor->toPlainText(), text); // even 5000 replacements undo in one step
}

void EditorSearchTest::replaceRangesAcrossLinesKeepsTheLineBreaks()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a\nb\na\nb")));
    const auto ranges =
        scribe::core::findAll(QStringLiteral("a\nb\na\nb"), QStringLiteral("a"), SearchOptions{});

    EditorSearch::replaceRanges(*editor, ranges, QStringLiteral("AA"));

    QCOMPARE(editor->toPlainText(), QStringLiteral("AA\nb\nAA\nb"));
    QCOMPARE(editor->lineCount(), 4);
}

void EditorSearchTest::replaceRangesMarksTheDocumentModified()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("a a")));
    QVERIFY(!editor->isModified());

    EditorSearch::replaceRanges(*editor, {{0, 1}}, QStringLiteral("b"));

    QVERIFY(editor->isModified());
}

void EditorSearchTest::highlightMarksTheMatchEvenWithoutFocus()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("one two three")));
    QVERIFY(!editor->hasFocus());

    EditorSearch::highlight(*editor, TextRange{4, 3});

    const auto marks = editor->extraSelections();
    QCOMPARE(marks.size(), 1);
    QCOMPARE(marks.first().cursor.selectedText(), QStringLiteral("two"));
    QVERIFY(marks.first().format.background().style() != Qt::NoBrush);
}

void EditorSearchTest::highlightNulloptRemovesTheMark()
{
    const std::unique_ptr<EditorWidget> editor(editorWith(QStringLiteral("one two")));
    EditorSearch::highlight(*editor, TextRange{0, 3});
    QCOMPARE(editor->extraSelections().size(), 1);

    EditorSearch::highlight(*editor, std::nullopt);

    QVERIFY(editor->extraSelections().isEmpty());
}

QTEST_MAIN(EditorSearchTest)

#include "EditorSearchTest.moc"
