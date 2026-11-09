#include "FileDialogs.h"

#include "ThemeManager.h"
#include "UnsavedChangesDialog.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QtTest>

#include <functional>

using scribe::app::UnsavedChangesDialog;

// The real unsaved-changes dialog, driven by simulated clicks and key presses.
class FileDialogsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void saveButtonMeansSave();
    void dontSaveButtonMeansDiscard();
    void cancelButtonMeansCancel();
    void escapeMeansCancel();
    void enterPicksTheDefaultButtonWhichIsSave();
    void dialogNamesTheDocumentAndWarnsAboutLoss();
    void dialogOffersExactlyThreeButtons();
    void buttonsAreInTheOrderOfTheDesign();
    void longNameIsShortenedToFitOneLine();
    void cardHasTheDesignWidthAndButtonHeight();
    void dialogHasNoSystemTitleBar();

private:
    // Runs the real dialog and calls `act` on it once it is on screen.
    static UnsavedChoice ask(const std::function<void(UnsavedChangesDialog&)>& act,
                             const QString& name = QStringLiteral("notes.txt"));
    static QString plain(const QString& text);
};

QString FileDialogsTest::plain(const QString& text)
{
    QString label = text;
    label.remove(QLatin1Char('&')); // drop the mnemonic marker
    return label;
}

UnsavedChoice FileDialogsTest::ask(const std::function<void(UnsavedChangesDialog&)>& act,
                                   const QString& name)
{
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &poll, [&] {
        auto* box = qobject_cast<UnsavedChangesDialog*>(QApplication::activeModalWidget());
        if (box == nullptr) {
            return; // the dialog is not on screen yet
        }
        poll.stop();
        act(*box);
        if (box->isVisible()) {
            box->reject(); // never leave a test hanging on a dialog the action did not close
        }
    });
    poll.start(10);

    return defaultFileDialogs().confirmDiscard(nullptr, name);
}

void FileDialogsTest::initTestCase()
{
    // The look of the dialog (button height, fonts) comes from the theme, as in the application.
    static scribe::app::ThemeManager themes(*qobject_cast<QApplication*>(QApplication::instance()));
    Q_UNUSED(themes);
}

void FileDialogsTest::saveButtonMeansSave()
{
    const UnsavedChoice choice = ask([](UnsavedChangesDialog& box) { box.saveButton()->click(); });
    QCOMPARE(choice, UnsavedChoice::Save);
}

void FileDialogsTest::dontSaveButtonMeansDiscard()
{
    const UnsavedChoice choice =
        ask([](UnsavedChangesDialog& box) { box.discardButton()->click(); });
    QCOMPARE(choice, UnsavedChoice::Discard);
}

void FileDialogsTest::cancelButtonMeansCancel()
{
    const UnsavedChoice choice =
        ask([](UnsavedChangesDialog& box) { box.cancelButton()->click(); });
    QCOMPARE(choice, UnsavedChoice::Cancel);
}

void FileDialogsTest::escapeMeansCancel()
{
    // Esc and the window's close button must never discard or save anything.
    const UnsavedChoice choice =
        ask([](UnsavedChangesDialog& box) { QTest::keyClick(&box, Qt::Key_Escape); });
    QCOMPARE(choice, UnsavedChoice::Cancel);
}

void FileDialogsTest::enterPicksTheDefaultButtonWhichIsSave()
{
    const UnsavedChoice choice = ask([](UnsavedChangesDialog& box) {
        QVERIFY(box.saveButton()->isDefault());
        QCOMPARE(box.focusWidget(), box.saveButton()); // hasFocus() needs an active window
        QTest::keyClick(&box, Qt::Key_Return);
    });
    QCOMPARE(choice, UnsavedChoice::Save);
}

void FileDialogsTest::dialogNamesTheDocumentAndWarnsAboutLoss()
{
    QString title;
    QString body;
    ask(
        [&](UnsavedChangesDialog& box) {
            title = box.titleLabel()->text();
            body = box.bodyLabel()->text();
        },
        QStringLiteral("report.txt"));

    QCOMPARE(title, QStringLiteral("Save changes to report.txt?"));
    QCOMPARE(body, QStringLiteral("Your changes will be lost if you don't save them."));
}

void FileDialogsTest::dialogOffersExactlyThreeButtons()
{
    int buttons = 0;
    ask([&buttons](UnsavedChangesDialog& box) {
        buttons = static_cast<int>(box.findChildren<QPushButton*>().size());
    });
    QCOMPARE(buttons, 3);
}

void FileDialogsTest::buttonsAreInTheOrderOfTheDesign()
{
    // Figma: Save, Don't save, Cancel, left to right, 8 px apart.
    QStringList names;
    int gap1 = 0;
    int gap2 = 0;
    ask([&](UnsavedChangesDialog& box) {
        for (const QPushButton* button :
             {box.saveButton(), box.discardButton(), box.cancelButton()}) {
            names << plain(button->text());
        }
        gap1 = box.discardButton()->x() - box.saveButton()->geometry().right() - 1;
        gap2 = box.cancelButton()->x() - box.discardButton()->geometry().right() - 1;
    });
    QCOMPARE(names, (QStringList{"Save", "Don't save", "Cancel"}));
    QCOMPARE(gap1, 8);
    QCOMPARE(gap2, 8);
}

void FileDialogsTest::longNameIsShortenedToFitOneLine()
{
    const QString name = QString("very-long-file-name-").repeated(12) + ".txt";
    QString title;
    int width = 0;
    int available = 0;
    ask(
        [&](UnsavedChangesDialog& box) {
            title = box.titleLabel()->text();
            width = box.titleLabel()->fontMetrics().horizontalAdvance(title);
            available = box.titleLabel()->width();
        },
        name);

    QVERIFY(title.startsWith("Save changes to "));
    QVERIFY(title.endsWith(".txt?")); // the end of a name matters most: the extension
    QVERIFY2(title.contains(QChar(0x2026)), qPrintable(title)); // an ellipsis in the middle
    QVERIFY2(width <= available + 1, qPrintable(QString("%1 > %2").arg(width).arg(available)));
}

void FileDialogsTest::cardHasTheDesignWidthAndButtonHeight()
{
    int cardWidth = 0;
    int saveHeight = 0;
    int titleLeft = 0;
    ask([&](UnsavedChangesDialog& box) {
        cardWidth = box.card()->width();
        saveHeight = box.saveButton()->height();
        titleLeft = box.titleLabel()->mapTo(box.card(), QPoint(0, 0)).x();
    });
    QCOMPARE(cardWidth, UnsavedChangesDialog::kCardWidth);
    QCOMPARE(saveHeight, 28);
    QCOMPARE(titleLeft, 25); // 1 px border + 24 px padding
}

void FileDialogsTest::dialogHasNoSystemTitleBar()
{
    bool frameless = false;
    ask([&](UnsavedChangesDialog& box) {
        frameless = box.windowFlags().testFlag(Qt::FramelessWindowHint);
    });
    QVERIFY(frameless);
}

QTEST_MAIN(FileDialogsTest)

#include "FileDialogsTest.moc"
