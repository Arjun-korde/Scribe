#include "EditorWidget.h"

#include <QFile>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QtTest>

using scribe::core::DocumentError;
using scribe::core::Encoding;
using scribe::core::EncodingInfo;
using scribe::core::LineEnding;
using scribe::editor::EditorWidget;

class EditorWidgetTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void newEditorIsEmptyAndClean();
    void typingMarksModified();
    void openLoadsTextAndStaysClean();
    void openOfMissingFileKeepsContent();
    void openDropsUndoHistory();
    void saveWritesTypedTextAndClearsModified();
    void saveKeepsLineEndingAndEncoding();
    void saveAsAdoptsPathAndName();
    void saveWithoutPathFails();
    void failedSaveStaysModified();
    void failedSaveThenUndoStillSavesTheWidgetText();
    void changingFormatCountsAsModified();
    void newFileResetsEverything();
    void stateChangedIsEmitted();

    void lineCountIsOnePlusTheLineBreaks();
    void aLongLineIsOneLineEvenWhenItWraps();
    void currentLineFollowsTheCaret();
    void goToLinePutsTheCaretAtTheStartOfThatLine();
    void goToLineWorksOnTheFirstAndLastLine();
    void goToLineRemovesAnySelection();
    void goToLineRejectsLinesOutsideTheDocumentAndChangesNothing();
    void goToLineDoesNotChangeTheText();
    void aFarJumpCentersTheLineOnScreen();
    void aJumpToAVisibleLineDoesNotScroll();

private:
    QString path(const QString& name) const { return m_dir->filePath(name); }
    void write(const QString& name, const QByteArray& bytes) const;
    QByteArray read(const QString& name) const;

    std::unique_ptr<QTemporaryDir> m_dir;
};

void EditorWidgetTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

void EditorWidgetTest::write(const QString& name, const QByteArray& bytes) const
{
    QFile file(path(name));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(bytes);
}

QByteArray EditorWidgetTest::read(const QString& name) const
{
    QFile file(path(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void EditorWidgetTest::newEditorIsEmptyAndClean()
{
    const EditorWidget editor;
    QVERIFY(editor.toPlainText().isEmpty());
    QVERIFY(!editor.isModified());
    QCOMPARE(editor.displayName(), QStringLiteral("Untitled"));
}

void EditorWidgetTest::typingMarksModified()
{
    EditorWidget editor;
    editor.insertPlainText(QStringLiteral("hello"));
    QVERIFY(editor.isModified());
}

void EditorWidgetTest::openLoadsTextAndStaysClean()
{
    write("a.txt", "one\r\ntwo");
    EditorWidget editor;
    QVERIFY(editor.openFile(path("a.txt")).ok());

    QCOMPARE(editor.toPlainText(), QStringLiteral("one\ntwo"));
    QVERIFY(!editor.isModified());
    QCOMPARE(editor.displayName(), QStringLiteral("a.txt"));
    QCOMPARE(editor.fileDocument().lineEnding(), LineEnding::Crlf);
}

void EditorWidgetTest::openOfMissingFileKeepsContent()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("keep"));

    const auto result = editor.openFile(path("missing.txt"));
    QCOMPARE(result.error, DocumentError::CannotRead);
    QCOMPARE(editor.toPlainText(), QStringLiteral("keep"));
    QVERIFY(editor.fileDocument().filePath().isEmpty());
}

void EditorWidgetTest::openDropsUndoHistory()
{
    write("a.txt", "loaded");
    EditorWidget editor;
    editor.insertPlainText(QStringLiteral("typed"));
    QVERIFY(editor.openFile(path("a.txt")).ok());

    editor.undo(); // must not bring back "typed" or empty the document
    QCOMPARE(editor.toPlainText(), QStringLiteral("loaded"));
}

void EditorWidgetTest::saveWritesTypedTextAndClearsModified()
{
    write("a.txt", "abc");
    EditorWidget editor;
    QVERIFY(editor.openFile(path("a.txt")).ok());
    editor.moveCursor(QTextCursor::End);
    editor.insertPlainText(QStringLiteral("def"));
    QVERIFY(editor.isModified());

    QVERIFY(editor.saveFile().ok());
    QVERIFY(!editor.isModified());
    QCOMPARE(read("a.txt"), QByteArray("abcdef"));
}

void EditorWidgetTest::saveKeepsLineEndingAndEncoding()
{
    write("a.txt", QByteArray::fromHex("FFFE") + QByteArray::fromHex("61000D000A006200"));
    EditorWidget editor;
    QVERIFY(editor.openFile(path("a.txt")).ok());
    editor.moveCursor(QTextCursor::End);
    editor.insertPlainText(QStringLiteral("\nc"));

    QVERIFY(editor.saveFile().ok());
    QCOMPARE(read("a.txt"), QByteArray::fromHex("FFFE61000D000A0062000D000A006300"));
}

void EditorWidgetTest::saveAsAdoptsPathAndName()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("hi"));
    QVERIFY(editor.saveFileAs(path("new.txt")).ok());

    QCOMPARE(editor.displayName(), QStringLiteral("new.txt"));
    QVERIFY(!editor.isModified());
    QCOMPARE(read("new.txt"), QByteArray("hi"));
}

void EditorWidgetTest::saveWithoutPathFails()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("hi"));
    QCOMPARE(editor.saveFile().error, DocumentError::CannotWrite);
    QVERIFY(editor.isModified());
}

void EditorWidgetTest::failedSaveStaysModified()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("hi"));
    QVERIFY(!editor.saveFileAs(path("no-such-folder/x.txt")).ok());
    QVERIFY(editor.isModified());
    QVERIFY(editor.fileDocument().filePath().isEmpty());
}

void EditorWidgetTest::failedSaveThenUndoStillSavesTheWidgetText()
{
    write("a.txt", "original");
    EditorWidget editor;
    QVERIFY(editor.openFile(path("a.txt")).ok());
    editor.moveCursor(QTextCursor::End);
    editor.insertPlainText(QStringLiteral("EDIT"));

    // A failed save: the Document now holds "originalEDIT" and stays modified.
    QVERIFY(!editor.saveFileAs(path("no-such-folder/x.txt")).ok());

    editor.undo(); // the widget is back to the text on disk
    QCOMPARE(editor.toPlainText(), QStringLiteral("original"));
    QVERIFY(editor.isModified()); // conservative: the Document is still marked modified

    QVERIFY(editor.saveFile().ok());
    QCOMPARE(read("a.txt"), QByteArray("original")); // not the stale "originalEDIT"
}

void EditorWidgetTest::changingFormatCountsAsModified()
{
    write("a.txt", "a\nb");
    EditorWidget editor;
    QVERIFY(editor.openFile(path("a.txt")).ok());
    QVERIFY(!editor.isModified());

    editor.setLineEnding(LineEnding::Crlf);
    QVERIFY(editor.isModified());
    QVERIFY(editor.saveFile().ok());
    QCOMPARE(read("a.txt"), QByteArray("a\r\nb"));

    editor.setEncoding({Encoding::Utf8, true});
    QVERIFY(editor.isModified());
    QVERIFY(editor.saveFile().ok());
    QCOMPARE(read("a.txt"), QByteArray::fromHex("EFBBBF") + "a\r\nb");
}

void EditorWidgetTest::newFileResetsEverything()
{
    write("a.txt", "abc");
    EditorWidget editor;
    QVERIFY(editor.openFile(path("a.txt")).ok());
    editor.insertPlainText(QStringLiteral("x"));

    editor.newFile();
    QVERIFY(editor.toPlainText().isEmpty());
    QVERIFY(!editor.isModified());
    QCOMPARE(editor.displayName(), QStringLiteral("Untitled"));
    QCOMPARE(editor.fileDocument().encoding(), (EncodingInfo{Encoding::Utf8, false}));
}

void EditorWidgetTest::stateChangedIsEmitted()
{
    EditorWidget editor;
    QSignalSpy spy(&editor, &EditorWidget::stateChanged);

    editor.insertPlainText(QStringLiteral("x"));
    QVERIFY(spy.count() >= 1);

    spy.clear();
    QVERIFY(editor.saveFileAs(path("s.txt")).ok());
    QVERIFY(spy.count() >= 1);

    spy.clear();
    editor.setLineEnding(LineEnding::Lf);
    QCOMPARE(spy.count(), 1);
}

namespace
{

QString numberedLines(int count)
{
    QStringList lines;
    for (int i = 1; i <= count; ++i) {
        lines << QStringLiteral("line %1").arg(i);
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace

void EditorWidgetTest::lineCountIsOnePlusTheLineBreaks()
{
    EditorWidget editor;
    QCOMPARE(editor.lineCount(), 1); // an empty document still has one (empty) line

    editor.setPlainText(QStringLiteral("a"));
    QCOMPARE(editor.lineCount(), 1);
    editor.setPlainText(QStringLiteral("a\nb\nc"));
    QCOMPARE(editor.lineCount(), 3);
    editor.setPlainText(QStringLiteral("a\nb\n")); // a trailing break starts an empty last line
    QCOMPARE(editor.lineCount(), 3);
    editor.setPlainText(QStringLiteral("\n\n"));
    QCOMPARE(editor.lineCount(), 3);
}

void EditorWidgetTest::aLongLineIsOneLineEvenWhenItWraps()
{
    EditorWidget editor;
    editor.resize(200, 200);
    editor.setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editor.setPlainText(QString(5000, QLatin1Char('x')) + QStringLiteral("\nsecond"));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    QCOMPARE(editor.lineCount(), 2); // wrapped on screen, but only two lines of text
}

void EditorWidgetTest::currentLineFollowsTheCaret()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("one\ntwo\nthree"));

    editor.moveCursor(QTextCursor::Start);
    QCOMPARE(editor.currentLine(), 1);
    editor.moveCursor(QTextCursor::Down);
    QCOMPARE(editor.currentLine(), 2);
    editor.moveCursor(QTextCursor::End);
    QCOMPARE(editor.currentLine(), 3);
}

void EditorWidgetTest::goToLinePutsTheCaretAtTheStartOfThatLine()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("alpha\nbeta\ngamma"));

    QVERIFY(editor.goToLine(2));

    QCOMPARE(editor.currentLine(), 2);
    QCOMPARE(editor.textCursor().positionInBlock(), 0); // column 1
    QCOMPARE(editor.textCursor().block().text(), QStringLiteral("beta"));
}

void EditorWidgetTest::goToLineWorksOnTheFirstAndLastLine()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("one\ntwo\nthree"));

    QVERIFY(editor.goToLine(3));
    QCOMPARE(editor.currentLine(), 3);
    QVERIFY(editor.goToLine(1));
    QCOMPARE(editor.currentLine(), 1);
    QCOMPARE(editor.textCursor().position(), 0);
}

void EditorWidgetTest::goToLineRemovesAnySelection()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("one\ntwo\nthree"));
    editor.selectAll();
    QVERIFY(editor.textCursor().hasSelection());

    QVERIFY(editor.goToLine(2));

    QVERIFY(!editor.textCursor().hasSelection());
}

void EditorWidgetTest::goToLineRejectsLinesOutsideTheDocumentAndChangesNothing()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("one\ntwo\nthree"));
    editor.goToLine(2);
    const int before = editor.textCursor().position();

    QVERIFY(!editor.goToLine(0));
    QVERIFY(!editor.goToLine(-3));
    QVERIFY(!editor.goToLine(4)); // one past the last line
    QVERIFY(!editor.goToLine(1000000));

    QCOMPARE(editor.textCursor().position(), before);
    QCOMPARE(editor.currentLine(), 2);
}

void EditorWidgetTest::goToLineDoesNotChangeTheText()
{
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("one\ntwo"));
    editor.document()->setModified(false);

    editor.goToLine(2);

    QCOMPARE(editor.toPlainText(), QStringLiteral("one\ntwo"));
    QVERIFY(!editor.isModified()); // moving the caret is not an edit
}

void EditorWidgetTest::aFarJumpCentersTheLineOnScreen()
{
    EditorWidget editor;
    editor.resize(400, 240);
    editor.setPlainText(numberedLines(1000));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    QCOMPARE(editor.verticalScrollBar()->value(), 0);

    QVERIFY(editor.goToLine(500));

    const QRect caret = editor.cursorRect();
    QVERIFY(editor.viewport()->rect().contains(caret)); // the line is on screen
    const int lineHeight = editor.fontMetrics().lineSpacing();
    const int distanceFromMiddle = qAbs(caret.center().y() - editor.viewport()->height() / 2);
    QVERIFY2(distanceFromMiddle <= 2 * lineHeight, qPrintable(QString::number(distanceFromMiddle)));
}

void EditorWidgetTest::aJumpToAVisibleLineDoesNotScroll()
{
    EditorWidget editor;
    editor.resize(400, 400);
    editor.setPlainText(numberedLines(1000));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.goToLine(1);
    QCOMPARE(editor.verticalScrollBar()->value(), 0);

    QVERIFY(editor.goToLine(3)); // already on screen

    QCOMPARE(editor.verticalScrollBar()->value(), 0); // the view stayed where it was
    QCOMPARE(editor.currentLine(), 3);
}

QTEST_MAIN(EditorWidgetTest)

#include "EditorWidgetTest.moc"
