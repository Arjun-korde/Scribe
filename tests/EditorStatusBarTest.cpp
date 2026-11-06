#include "EditorStatusBar.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QFile>
#include <QLabel>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QtTest>

using scribe::core::Encoding;
using scribe::core::LineEnding;
using scribe::editor::EditorStatusBar;
using scribe::editor::EditorTabs;

class EditorStatusBarTest : public QObject
{
    Q_OBJECT

private slots:
    void textHelpersUseTheDesignWording();
    void characterCountIsGroupedAndHandlesOne();

    void freshTabShowsTheDefaults();
    void typingUpdatesPositionAndCount();
    void movingTheCaretUpdatesThePosition();
    void positionCountsLinesAndColumnsFromOne();
    void lineBreaksCountAsOneCharacter();
    void openedFileShowsItsEncodingAndLineEnding();
    void changingEncodingAndLineEndingUpdatesTheBar();
    void zoomUpdatesThePercentage();
    void followsTheActiveTab();
    void keepsWorkingAfterTheFollowedTabIsClosed();
    void segmentsHaveAccessibleNames();

private:
    QString makeFile(const QString& name, const QByteArray& bytes) const;
    std::unique_ptr<QTemporaryDir> m_dir = std::make_unique<QTemporaryDir>();
};

QString EditorStatusBarTest::makeFile(const QString& name, const QByteArray& bytes) const
{
    QFile file(m_dir->filePath(name));
    file.open(QIODevice::WriteOnly);
    file.write(bytes);
    return file.fileName();
}

void EditorStatusBarTest::textHelpersUseTheDesignWording()
{
    QCOMPARE(EditorStatusBar::positionText(3, 14), QStringLiteral("Ln 3, Col 14"));
    QCOMPARE(EditorStatusBar::zoomText(100), QStringLiteral("100%"));
    QCOMPARE(EditorStatusBar::zoomText(10), QStringLiteral("10%"));
}

void EditorStatusBarTest::characterCountIsGroupedAndHandlesOne()
{
    const QLocale us(QLocale::English, QLocale::UnitedStates);
    QCOMPARE(EditorStatusBar::charactersText(0, us), QStringLiteral("0 chars"));
    QCOMPARE(EditorStatusBar::charactersText(1, us), QStringLiteral("1 char"));
    QCOMPARE(EditorStatusBar::charactersText(2, us), QStringLiteral("2 chars"));
    QCOMPARE(EditorStatusBar::charactersText(1204, us), QStringLiteral("1,204 chars"));
    QCOMPARE(EditorStatusBar::charactersText(1234567, us), QStringLiteral("1,234,567 chars"));
}

void EditorStatusBarTest::freshTabShowsTheDefaults()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    QCOMPARE(bar.position(), QStringLiteral("Ln 1, Col 1"));
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(0));
    QCOMPARE(bar.encoding(), QStringLiteral("UTF-8"));
    QCOMPARE(bar.lineEnding(), QStringLiteral("CRLF"));
    QCOMPARE(bar.zoom(), QStringLiteral("100%"));
}

void EditorStatusBarTest::typingUpdatesPositionAndCount()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    tabs.currentEditor()->insertPlainText(QStringLiteral("hello"));

    QCOMPARE(bar.position(), QStringLiteral("Ln 1, Col 6")); // the caret is after the 5th letter
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(5));
}

void EditorStatusBarTest::movingTheCaretUpdatesThePosition()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    tabs.currentEditor()->insertPlainText(QStringLiteral("hello"));

    tabs.currentEditor()->moveCursor(QTextCursor::StartOfLine);
    QCOMPARE(bar.position(), QStringLiteral("Ln 1, Col 1"));

    tabs.currentEditor()->moveCursor(QTextCursor::Right);
    tabs.currentEditor()->moveCursor(QTextCursor::Right);
    QCOMPARE(bar.position(), QStringLiteral("Ln 1, Col 3"));
}

void EditorStatusBarTest::positionCountsLinesAndColumnsFromOne()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    tabs.currentEditor()->insertPlainText(QStringLiteral("one\ntwo\nthree"));

    QCOMPARE(bar.position(), QStringLiteral("Ln 3, Col 6")); // end of "three"

    tabs.currentEditor()->moveCursor(QTextCursor::Up);
    QCOMPARE(bar.position(), QStringLiteral("Ln 2, Col 4")); // end of "two"

    QTextCursor cursor = tabs.currentEditor()->textCursor();
    cursor.setPosition(0);
    tabs.currentEditor()->setTextCursor(cursor);
    QCOMPARE(bar.position(), QStringLiteral("Ln 1, Col 1"));
}

void EditorStatusBarTest::lineBreaksCountAsOneCharacter()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    tabs.currentEditor()->insertPlainText(QStringLiteral("ab\ncd")); // 2 + break + 2

    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(5));
}

void EditorStatusBarTest::openedFileShowsItsEncodingAndLineEnding()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    const QString file = makeFile("a.txt", QByteArray::fromHex("EFBBBF") + "x\ny\n");

    QVERIFY(tabs.openFile(file).ok());

    QCOMPARE(bar.encoding(), QStringLiteral("UTF-8 with BOM"));
    QCOMPARE(bar.lineEnding(), QStringLiteral("LF"));
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(4)); // x, break, y, break
}

void EditorStatusBarTest::changingEncodingAndLineEndingUpdatesTheBar()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    tabs.currentEditor()->setEncoding({Encoding::Utf16Le, true});
    QCOMPARE(bar.encoding(), QStringLiteral("UTF-16 LE"));

    tabs.currentEditor()->setLineEnding(LineEnding::Cr);
    QCOMPARE(bar.lineEnding(), QStringLiteral("CR"));
}

void EditorStatusBarTest::zoomUpdatesThePercentage()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    tabs.zoomBy(3);
    QCOMPARE(bar.zoom(), QStringLiteral("130%"));
    tabs.resetZoom();
    QCOMPARE(bar.zoom(), QStringLiteral("100%"));
}

void EditorStatusBarTest::followsTheActiveTab()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    tabs.currentEditor()->insertPlainText(QStringLiteral("first tab text"));

    tabs.addEditor();
    tabs.currentEditor()->setEncoding({Encoding::Ansi, false});
    tabs.currentEditor()->insertPlainText(QStringLiteral("hi"));
    QCOMPARE(bar.encoding(), QStringLiteral("ANSI"));
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(2));

    tabs.setCurrentIndex(0);
    QCOMPARE(bar.encoding(), QStringLiteral("UTF-8"));
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(14));
    QCOMPARE(bar.position(), QStringLiteral("Ln 1, Col 15"));

    // Typing in a background tab must not disturb what the bar shows for the active one.
    tabs.editorAt(1)->insertPlainText(QStringLiteral("more"));
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(14));
}

void EditorStatusBarTest::keepsWorkingAfterTheFollowedTabIsClosed()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    tabs.addEditor();
    tabs.currentEditor()->insertPlainText(QStringLiteral("gone soon"));

    tabs.closeTab(1);
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(0));

    tabs.currentEditor()->insertPlainText(QStringLiteral("abc"));
    QCOMPARE(bar.characters(), EditorStatusBar::charactersText(3));
}

void EditorStatusBarTest::segmentsHaveAccessibleNames()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    QStringList names;
    for (const QLabel* label : bar.findChildren<QLabel*>()) {
        names << label->accessibleName();
    }
    names.sort();
    QCOMPARE(names, (QStringList{"Character count", "Cursor position", "Encoding", "Line ending",
                                 "Zoom"}));
}

QTEST_MAIN(EditorStatusBarTest)

#include "EditorStatusBarTest.moc"
