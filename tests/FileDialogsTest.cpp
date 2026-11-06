#include "FileDialogs.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QtTest>

#include <functional>

// The real unsaved-changes dialog, driven by simulated clicks and key presses.
class FileDialogsTest : public QObject
{
    Q_OBJECT

private slots:
    void saveButtonMeansSave();
    void dontSaveButtonMeansDiscard();
    void cancelButtonMeansCancel();
    void escapeMeansCancel();
    void enterPicksTheDefaultButtonWhichIsSave();
    void dialogNamesTheDocumentAndWarnsAboutLoss();
    void dialogOffersExactlyThreeButtons();

private:
    // Runs the real dialog and calls `act` on it once it is on screen.
    static UnsavedChoice ask(const std::function<void(QMessageBox&)>& act,
                             const QString& name = QStringLiteral("notes.txt"));
    static QAbstractButton* buttonNamed(const QMessageBox& box, const QString& text);
};

QAbstractButton* FileDialogsTest::buttonNamed(const QMessageBox& box, const QString& text)
{
    for (QAbstractButton* button : box.buttons()) {
        QString label = button->text();
        label.remove(QLatin1Char('&')); // drop the mnemonic marker
        if (label == text) {
            return button;
        }
    }
    return nullptr;
}

UnsavedChoice FileDialogsTest::ask(const std::function<void(QMessageBox&)>& act,
                                   const QString& name)
{
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &poll, [&] {
        auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
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

void FileDialogsTest::saveButtonMeansSave()
{
    const UnsavedChoice choice = ask([](QMessageBox& box) {
        QAbstractButton* button = buttonNamed(box, QStringLiteral("Save"));
        QVERIFY(button != nullptr);
        button->click();
    });
    QCOMPARE(choice, UnsavedChoice::Save);
}

void FileDialogsTest::dontSaveButtonMeansDiscard()
{
    const UnsavedChoice choice = ask([](QMessageBox& box) {
        QAbstractButton* button = buttonNamed(box, QStringLiteral("Don't Save"));
        QVERIFY(button != nullptr);
        button->click();
    });
    QCOMPARE(choice, UnsavedChoice::Discard);
}

void FileDialogsTest::cancelButtonMeansCancel()
{
    const UnsavedChoice choice = ask([](QMessageBox& box) {
        QAbstractButton* button = buttonNamed(box, QStringLiteral("Cancel"));
        QVERIFY(button != nullptr);
        button->click();
    });
    QCOMPARE(choice, UnsavedChoice::Cancel);
}

void FileDialogsTest::escapeMeansCancel()
{
    // Esc and the window's close button must never discard or save anything.
    const UnsavedChoice choice =
        ask([](QMessageBox& box) { QTest::keyClick(&box, Qt::Key_Escape); });
    QCOMPARE(choice, UnsavedChoice::Cancel);
}

void FileDialogsTest::enterPicksTheDefaultButtonWhichIsSave()
{
    const UnsavedChoice choice = ask([](QMessageBox& box) {
        QCOMPARE(box.defaultButton(), buttonNamed(box, QStringLiteral("Save")));
        box.defaultButton()->click(); // what pressing Enter does on the default button
    });
    QCOMPARE(choice, UnsavedChoice::Save);
}

void FileDialogsTest::dialogNamesTheDocumentAndWarnsAboutLoss()
{
    QString text;
    QString informative;
    ask(
        [&](QMessageBox& box) {
            text = box.text();
            informative = box.informativeText();
        },
        QStringLiteral("report.txt"));

    QVERIFY2(text.contains(QStringLiteral("report.txt")), qPrintable(text));
    QVERIFY(!informative.isEmpty());
}

void FileDialogsTest::dialogOffersExactlyThreeButtons()
{
    int buttons = 0;
    ask([&buttons](QMessageBox& box) { buttons = static_cast<int>(box.buttons().size()); });
    QCOMPARE(buttons, 3);
}

QTEST_MAIN(FileDialogsTest)

#include "FileDialogsTest.moc"
