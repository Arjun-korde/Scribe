#include "GoToLineDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QtTest>

// The Go To Line dialog, driven with real key presses: what is accepted, what is refused, and
// what the user is told.
class GoToLineDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void opensOnTheCurrentLineSelectedSoTypingReplacesIt();
    void aValidNumberEnablesGoAndShowsNoHint();
    void aNumberPastTheLastLineIsInvalid();
    void zeroAndEmptyAreInvalid();
    void theHintExplainsTheAllowedRange();
    void theFieldIsMarkedInvalidForTheThemeToStyle();
    void theHintIsAlsoAvailableToScreenReaders();
    void lettersAndSignsCannotBeTyped();
    void enterDoesNothingWhileTheNumberIsInvalid();
    void enterWithAValidNumberAccepts();
    void escapeCancels();
    void lineReturnsTheTypedNumberOnlyWhenValid();
    void theLastLineItselfIsValid();
    void aOneLineDocumentOnlyAllowsLineOne();
    void fixingAnInvalidNumberClearsTheError();
    void theFieldHasAccessibleNames();
    void theGoButtonIsTheDefault();

private:
    static QLineEdit* field(const GoToLineDialog& dialog) { return dialog.findChild<QLineEdit*>(); }
    static QPushButton* goButton(const GoToLineDialog& dialog)
    {
        return dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok);
    }
    static QString hint(const GoToLineDialog& dialog)
    {
        // The hint is the label that is not the prompt in front of the field.
        const auto labels = dialog.findChildren<QLabel*>();
        return labels.last()->text();
    }
    static void type(const GoToLineDialog& dialog, const QString& text)
    {
        QTest::keyClicks(field(dialog), text);
    }
    // Replaces the whole field, like selecting everything and typing.
    static void typeInstead(const GoToLineDialog& dialog, const QString& text)
    {
        field(dialog)->selectAll();
        QTest::keyClicks(field(dialog), text);
    }
};

void GoToLineDialogTest::opensOnTheCurrentLineSelectedSoTypingReplacesIt()
{
    GoToLineDialog dialog(42, 300);
    dialog.show();

    QCOMPARE(field(dialog)->text(), QStringLiteral("42"));
    QCOMPARE(field(dialog)->selectedText(), QStringLiteral("42"));
    QVERIFY(goButton(dialog)->isEnabled()); // the current line is valid

    type(dialog, "7"); // replaces the selected "42"
    QCOMPARE(field(dialog)->text(), QStringLiteral("7"));
    QCOMPARE(dialog.line(), 7);
}

void GoToLineDialogTest::aValidNumberEnablesGoAndShowsNoHint()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();

    typeInstead(dialog, "299");

    QVERIFY(goButton(dialog)->isEnabled());
    QVERIFY(hint(dialog).isEmpty());
    QCOMPARE(dialog.line(), 299);
}

void GoToLineDialogTest::aNumberPastTheLastLineIsInvalid()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();

    typeInstead(dialog, "310");

    QVERIFY(!goButton(dialog)->isEnabled());
    QCOMPARE(dialog.line(), std::nullopt);
    QCOMPARE(field(dialog)->text(), QStringLiteral("310")); // it is not clamped or corrected
}

void GoToLineDialogTest::zeroAndEmptyAreInvalid()
{
    GoToLineDialog dialog(5, 300);
    dialog.show();

    typeInstead(dialog, "0");
    QVERIFY(!goButton(dialog)->isEnabled());

    field(dialog)->clear();
    QVERIFY(!goButton(dialog)->isEnabled());
    QVERIFY(!hint(dialog).isEmpty()); // an empty field gets the same explanation
}

void GoToLineDialogTest::theHintExplainsTheAllowedRange()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();

    typeInstead(dialog, "310");

    QCOMPARE(hint(dialog), QStringLiteral("Enter a number from 1 to 300"));
    QCOMPARE(GoToLineDialog::hintText(12), QStringLiteral("Enter a number from 1 to 12"));
}

void GoToLineDialogTest::theFieldIsMarkedInvalidForTheThemeToStyle()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();
    QCOMPARE(field(dialog)->property("invalid").toBool(), false);

    typeInstead(dialog, "310");
    QCOMPARE(field(dialog)->property("invalid").toBool(), true);

    typeInstead(dialog, "30");
    QCOMPARE(field(dialog)->property("invalid").toBool(), false);
}

void GoToLineDialogTest::theHintIsAlsoAvailableToScreenReaders()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();

    typeInstead(dialog, "310");
    QCOMPARE(field(dialog)->accessibleDescription(),
             QStringLiteral("Enter a number from 1 to 300"));

    typeInstead(dialog, "5");
    QVERIFY(field(dialog)->accessibleDescription().isEmpty());
}

void GoToLineDialogTest::lettersAndSignsCannotBeTyped()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();

    typeInstead(dialog, "1a2-b3+.4 ");

    QCOMPARE(field(dialog)->text(), QStringLiteral("1234"));
}

void GoToLineDialogTest::enterDoesNothingWhileTheNumberIsInvalid()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();
    typeInstead(dialog, "310");

    QTest::keyClick(field(dialog), Qt::Key_Return);
    QTest::keyClick(field(dialog), Qt::Key_Enter);

    QVERIFY(dialog.isVisible()); // still open, so the user can correct the number
    QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected)); // nothing was accepted
}

void GoToLineDialogTest::enterWithAValidNumberAccepts()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();
    typeInstead(dialog, "120");

    QTest::keyClick(field(dialog), Qt::Key_Return);

    QVERIFY(!dialog.isVisible());
    QCOMPARE(dialog.result(), static_cast<int>(QDialog::Accepted));
    QCOMPARE(dialog.line(), 120);
}

void GoToLineDialogTest::escapeCancels()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();
    typeInstead(dialog, "120");

    QTest::keyClick(&dialog, Qt::Key_Escape);

    QVERIFY(!dialog.isVisible());
    QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected));
}

void GoToLineDialogTest::lineReturnsTheTypedNumberOnlyWhenValid()
{
    GoToLineDialog dialog(10, 50);
    dialog.show();
    QCOMPARE(dialog.line(), 10);

    typeInstead(dialog, "51");
    QCOMPARE(dialog.line(), std::nullopt);
    typeInstead(dialog, "50");
    QCOMPARE(dialog.line(), 50);
}

void GoToLineDialogTest::theLastLineItselfIsValid()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();

    typeInstead(dialog, "300");
    QVERIFY(goButton(dialog)->isEnabled());
    typeInstead(dialog, "301");
    QVERIFY(!goButton(dialog)->isEnabled());
}

void GoToLineDialogTest::aOneLineDocumentOnlyAllowsLineOne()
{
    GoToLineDialog dialog(1, 1);
    dialog.show();
    QVERIFY(goButton(dialog)->isEnabled());

    typeInstead(dialog, "2");
    QVERIFY(!goButton(dialog)->isEnabled());
    QCOMPARE(hint(dialog), QStringLiteral("Enter a number from 1 to 1"));
}

void GoToLineDialogTest::fixingAnInvalidNumberClearsTheError()
{
    GoToLineDialog dialog(1, 300);
    dialog.show();
    typeInstead(dialog, "310");
    QVERIFY(!goButton(dialog)->isEnabled());

    QTest::keyClick(field(dialog), Qt::Key_Backspace); // "31"

    QVERIFY(goButton(dialog)->isEnabled());
    QVERIFY(hint(dialog).isEmpty());
    QCOMPARE(dialog.line(), 31);
}

void GoToLineDialogTest::theFieldHasAccessibleNames()
{
    GoToLineDialog dialog(1, 300);
    QCOMPARE(field(dialog)->accessibleName(), QStringLiteral("Line number"));
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Go To Line"));
}

void GoToLineDialogTest::theGoButtonIsTheDefault()
{
    GoToLineDialog dialog(1, 300);
    QVERIFY(goButton(dialog)->isDefault());
    QCOMPARE(goButton(dialog)->text(), QStringLiteral("Go"));
}

QTEST_MAIN(GoToLineDialogTest)

#include "GoToLineDialogTest.moc"
