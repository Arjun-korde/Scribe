#include "EditorStatusBar.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QFile>
#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QToolButton>
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

    void clickingThePositionSegmentReportsItself();
    void clickingTheEncodingSegmentReportsItself();
    void clickingTheLineEndingSegmentReportsItself();
    void onlyPositionEncodingAndLineEndingAreClickable();
    void clickingDoesNotTakeFocusFromTheEditor();
    void clickableSegmentsExplainThemselvesInATooltip();

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
    for (const QToolButton* button : bar.findChildren<QToolButton*>()) {
        names << button->accessibleName(); // the encoding and line ending segments
    }
    names.sort();
    QCOMPARE(names, (QStringList{"Character count", "Cursor position", "Encoding", "Line ending",
                                 "Zoom"}));
}

namespace
{

// The status bar's buttons are found by their accessible name, as a screen reader would.
QToolButton* segmentButton(const EditorStatusBar& bar, const QString& accessibleName)
{
    for (QToolButton* button : bar.findChildren<QToolButton*>()) {
        if (button->accessibleName() == accessibleName) {
            return button;
        }
    }
    return nullptr;
}

} // namespace

void EditorStatusBarTest::clickingThePositionSegmentReportsItself()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    QSignalSpy positionSpy(&bar, &EditorStatusBar::positionClicked);
    QSignalSpy encodingSpy(&bar, &EditorStatusBar::encodingClicked);
    QToolButton* button = segmentButton(bar, "Cursor position");
    QVERIFY(button != nullptr);

    button->click();

    QCOMPARE(positionSpy.count(), 1);
    QCOMPARE(encodingSpy.count(), 0);
    QCOMPARE(positionSpy.first().first().value<QWidget*>(), button);
}

void EditorStatusBarTest::clickingTheEncodingSegmentReportsItself()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    QSignalSpy encodingSpy(&bar, &EditorStatusBar::encodingClicked);
    QSignalSpy lineEndingSpy(&bar, &EditorStatusBar::lineEndingClicked);
    QToolButton* button = segmentButton(bar, "Encoding");
    QVERIFY(button != nullptr);

    button->click();

    QCOMPARE(encodingSpy.count(), 1);
    QCOMPARE(lineEndingSpy.count(), 0);
    QCOMPARE(encodingSpy.first().first().value<QWidget*>(), button); // the anchor for a menu
}

void EditorStatusBarTest::clickingTheLineEndingSegmentReportsItself()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);
    QSignalSpy encodingSpy(&bar, &EditorStatusBar::encodingClicked);
    QSignalSpy lineEndingSpy(&bar, &EditorStatusBar::lineEndingClicked);
    QToolButton* button = segmentButton(bar, "Line ending");
    QVERIFY(button != nullptr);

    button->click();

    QCOMPARE(lineEndingSpy.count(), 1);
    QCOMPARE(encodingSpy.count(), 0);
    QCOMPARE(lineEndingSpy.first().first().value<QWidget*>(), button);
}

void EditorStatusBarTest::onlyPositionEncodingAndLineEndingAreClickable()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    QStringList buttons;
    for (const QToolButton* button : bar.findChildren<QToolButton*>()) {
        buttons << button->accessibleName();
    }
    buttons.sort();
    QCOMPARE(buttons, (QStringList{"Cursor position", "Encoding", "Line ending"}));
}

void EditorStatusBarTest::clickingDoesNotTakeFocusFromTheEditor()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    // A click on a segment must leave the caret in the text, ready for typing.
    for (const QToolButton* button : bar.findChildren<QToolButton*>()) {
        QCOMPARE(button->focusPolicy(), Qt::NoFocus);
    }
}

void EditorStatusBarTest::clickableSegmentsExplainThemselvesInATooltip()
{
    EditorTabs tabs;
    const EditorStatusBar bar(tabs);

    QCOMPARE(segmentButton(bar, "Cursor position")->toolTip(), QStringLiteral("Go to line"));
    QVERIFY(!segmentButton(bar, "Encoding")->toolTip().isEmpty());
    QVERIFY(!segmentButton(bar, "Line ending")->toolTip().isEmpty());
}

QTEST_MAIN(EditorStatusBarTest)

#include "EditorStatusBarTest.moc"
