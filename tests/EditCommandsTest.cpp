#include "EditCommands.h"

#include "CommandRegistry.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QTextCursor>
#include <QtTest>

using scribe::core::CommandRegistry;
using scribe::editor::EditCommands;
using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;

class EditCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void registersEightCommandsWithTheirShortcuts();
    void undoAndRedoFollowTheUndoStack();
    void undoRedoRoundTrip();
    void cutCopyDeleteNeedASelection();
    void copyPutsTheSelectionOnTheClipboard();
    void cutRemovesAndCopies();
    void deleteRemovesOnlyTheSelection();
    void pasteNeedsClipboardTextAndInsertsIt();
    void selectAllIsAlwaysAvailable();
    void insertDateTimeUsesTheClockAndIsOneUndoStep();
    void timestampIsTimeThenShortDate();
    void commandsActOnTheActiveTabOnly();
    void disabledCommandsDoNothing();

private:
    // Each test gets a fresh registry bound to fresh tabs and a fixed clock.
    struct Fixture
    {
        EditorTabs tabs;
        CommandRegistry registry;

        Fixture()
        {
            EditCommands::registerIn(
                registry, tabs, [] { return QDateTime(QDate(2026, 11, 6), QTime(15, 45, 30)); });
        }

        EditorWidget* editor() { return tabs.currentEditor(); }
        bool enabled(QLatin1String id) const { return registry.isEnabled(id); }
        bool run(QLatin1String id) const { return registry.run(id); }
    };

    static void type(EditorWidget* editor, const QString& text)
    {
        editor->moveCursor(QTextCursor::End);
        editor->insertPlainText(text);
    }

    // The real clipboard is shared with the user's session, so put back what was there.
    QString m_savedClipboardText;
};

void EditCommandsTest::init()
{
    m_savedClipboardText = QApplication::clipboard()->text();
    QApplication::clipboard()->clear();
}

void EditCommandsTest::cleanup()
{
    QApplication::clipboard()->setText(m_savedClipboardText);
}

void EditCommandsTest::registersEightCommandsWithTheirShortcuts()
{
    Fixture f;
    QCOMPARE(f.registry.commands().size(), size_t{8});

    const QList<std::pair<QLatin1String, QString>> expected = {
        {EditCommands::kUndo, "Ctrl+Z"},      {EditCommands::kRedo, "Ctrl+Y"},
        {EditCommands::kCut, "Ctrl+X"},       {EditCommands::kCopy, "Ctrl+C"},
        {EditCommands::kPaste, "Ctrl+V"},     {EditCommands::kDelete, "Delete"},
        {EditCommands::kSelectAll, "Ctrl+A"}, {EditCommands::kInsertDateTime, "F5"},
    };
    for (const auto& [id, shortcut] : expected) {
        const auto* command = f.registry.find(id);
        if (command == nullptr) {
            QFAIL(qPrintable(id));
        }
        QCOMPARE(command->shortcut, shortcut);
        QVERIFY2(!command->title.isEmpty(), qPrintable(id));
    }
}

void EditCommandsTest::undoAndRedoFollowTheUndoStack()
{
    Fixture f;
    QVERIFY(!f.enabled(EditCommands::kUndo)); // a fresh document has nothing to undo
    QVERIFY(!f.enabled(EditCommands::kRedo));

    type(f.editor(), "abc");
    QVERIFY(f.enabled(EditCommands::kUndo));
    QVERIFY(!f.enabled(EditCommands::kRedo));

    QVERIFY(f.run(EditCommands::kUndo));
    QVERIFY(f.enabled(EditCommands::kRedo));
    QVERIFY(!f.enabled(EditCommands::kUndo));
}

void EditCommandsTest::undoRedoRoundTrip()
{
    Fixture f;
    type(f.editor(), "hello");

    QVERIFY(f.run(EditCommands::kUndo));
    QCOMPARE(f.editor()->toPlainText(), QString());
    QVERIFY(f.run(EditCommands::kRedo));
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("hello"));
}

void EditCommandsTest::cutCopyDeleteNeedASelection()
{
    Fixture f;
    type(f.editor(), "text");
    f.editor()->moveCursor(QTextCursor::End); // caret only, no selection

    QVERIFY(!f.enabled(EditCommands::kCut));
    QVERIFY(!f.enabled(EditCommands::kCopy));
    QVERIFY(!f.enabled(EditCommands::kDelete));

    f.editor()->selectAll();
    QVERIFY(f.enabled(EditCommands::kCut));
    QVERIFY(f.enabled(EditCommands::kCopy));
    QVERIFY(f.enabled(EditCommands::kDelete));
}

void EditCommandsTest::copyPutsTheSelectionOnTheClipboard()
{
    Fixture f;
    type(f.editor(), "clip me");
    f.editor()->selectAll();

    QVERIFY(f.run(EditCommands::kCopy));
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("clip me"));
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("clip me")); // copy leaves the text alone
}

void EditCommandsTest::cutRemovesAndCopies()
{
    Fixture f;
    type(f.editor(), "move me");
    f.editor()->selectAll();

    QVERIFY(f.run(EditCommands::kCut));
    QCOMPARE(f.editor()->toPlainText(), QString());
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("move me"));
}

void EditCommandsTest::deleteRemovesOnlyTheSelection()
{
    Fixture f;
    type(f.editor(), "abcdef");
    QTextCursor cursor = f.editor()->textCursor();
    cursor.setPosition(2);
    cursor.setPosition(4, QTextCursor::KeepAnchor); // "cd"
    f.editor()->setTextCursor(cursor);

    QVERIFY(f.run(EditCommands::kDelete));
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("abef"));
    QCOMPARE(f.editor()->textCursor().position(), 2);
    QVERIFY(QApplication::clipboard()->text().isEmpty()); // delete must not touch the clipboard
}

void EditCommandsTest::pasteNeedsClipboardTextAndInsertsIt()
{
    Fixture f;
    QVERIFY(!f.enabled(EditCommands::kPaste)); // the clipboard was cleared in init()
    QVERIFY(!f.run(EditCommands::kPaste));

    QApplication::clipboard()->setText(QStringLiteral("pasted"));
    QVERIFY(f.enabled(EditCommands::kPaste));
    type(f.editor(), "x ");
    QVERIFY(f.run(EditCommands::kPaste));
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("x pasted"));
}

void EditCommandsTest::selectAllIsAlwaysAvailable()
{
    Fixture f;
    QVERIFY(f.enabled(EditCommands::kSelectAll)); // even in an empty document
    type(f.editor(), "one\ntwo");
    f.editor()->moveCursor(QTextCursor::Start);

    QVERIFY(f.run(EditCommands::kSelectAll));
    QCOMPARE(f.editor()->textCursor().selectedText().size(), 7); // line break included
}

void EditCommandsTest::insertDateTimeUsesTheClockAndIsOneUndoStep()
{
    Fixture f;
    type(f.editor(), "at: ");
    const QString expected =
        EditCommands::timestamp(QDateTime(QDate(2026, 11, 6), QTime(15, 45, 30)));

    QVERIFY(f.run(EditCommands::kInsertDateTime));
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("at: ") + expected);

    // One undo removes the whole stamp, not a character at a time.
    f.editor()->undo();
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("at: "));
}

void EditCommandsTest::timestampIsTimeThenShortDate()
{
    const QDateTime when(QDate(2026, 11, 6), QTime(15, 45, 30));
    const QLocale us(QLocale::English, QLocale::UnitedStates);

    const QString stamp = EditCommands::timestamp(when, us);
    QCOMPARE(stamp, us.toString(when.time(), QLocale::ShortFormat) + QLatin1Char(' ') +
                        us.toString(when.date(), QLocale::ShortFormat));
    QVERIFY2(stamp.contains(QStringLiteral("3:45")), qPrintable(stamp));
    QVERIFY(stamp.indexOf(QStringLiteral("3:45")) <
            stamp.indexOf(QStringLiteral("11"))); // time first
}

void EditCommandsTest::commandsActOnTheActiveTabOnly()
{
    Fixture f;
    type(f.editor(), "first tab");
    f.tabs.addEditor();
    type(f.editor(), "second tab");

    QVERIFY(f.run(EditCommands::kSelectAll));
    QVERIFY(f.run(EditCommands::kDelete));
    QCOMPARE(f.tabs.editorAt(1)->toPlainText(), QString());
    QCOMPARE(f.tabs.editorAt(0)->toPlainText(), QStringLiteral("first tab"));

    // Availability follows the active tab too: tab 0 has undo history, the empty tab 1 has its own.
    f.tabs.setCurrentIndex(0);
    QVERIFY(!f.enabled(EditCommands::kCut)); // tab 0 has no selection, tab 1 had one
}

void EditCommandsTest::disabledCommandsDoNothing()
{
    Fixture f;
    type(f.editor(), "keep");
    f.editor()->moveCursor(QTextCursor::End);

    QVERIFY(!f.run(EditCommands::kCut));
    QVERIFY(!f.run(EditCommands::kDelete));
    QVERIFY(!f.run(EditCommands::kRedo));
    QCOMPARE(f.editor()->toPlainText(), QStringLiteral("keep"));
    QVERIFY(QApplication::clipboard()->text().isEmpty());
}

QTEST_MAIN(EditCommandsTest)

#include "EditCommandsTest.moc"
