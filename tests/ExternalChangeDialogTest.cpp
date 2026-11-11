#include "ExternalChangeDialog.h"

#include "FileDialogs.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QtTest>

#include <functional>

using scribe::app::ExternalChangeDialog;

// The real "changed on disk" dialog, driven by simulated clicks and key presses.
class ExternalChangeDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void reloadButtonMeansReload();
    void keepCurrentButtonMeansKeep();
    void escapeMeansKeep();
    void enterReloadsWhenThereAreNoUnsavedChanges();
    void enterKeepsWhenReloadWouldLoseEdits();
    void deletedFileOffersCloseTabAndKeep();
    void closeTabMeansClose();
    void enterKeepsADeletedFile();
    void textsDescribeEachSituation();
    void longNameIsShortenedToFitOneLine();
    void dialogHasNoSystemTitleBar();
    void defaultDialogsAreWiredUp();

private:
    // Runs the real dialog and calls `act` on it once it is on screen.
    static ExternalChangeChoice ask(const std::function<void(ExternalChangeDialog&)>& act,
                                    ExternalChangeKind kind = ExternalChangeKind::Changed,
                                    bool unsaved = false,
                                    const QString& name = QStringLiteral("notes.txt"));
};

ExternalChangeChoice
ExternalChangeDialogTest::ask(const std::function<void(ExternalChangeDialog&)>& act,
                              ExternalChangeKind kind, bool unsaved, const QString& name)
{
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &poll, [&] {
        auto* box = qobject_cast<ExternalChangeDialog*>(QApplication::activeModalWidget());
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

    return defaultFileDialogs().confirmExternalChange(nullptr, name, kind, unsaved);
}

void ExternalChangeDialogTest::initTestCase()
{
    static scribe::app::ThemeManager themes(*qobject_cast<QApplication*>(QApplication::instance()));
    Q_UNUSED(themes);
}

void ExternalChangeDialogTest::reloadButtonMeansReload()
{
    QCOMPARE(ask([](ExternalChangeDialog& box) { box.primaryButton()->click(); }),
             ExternalChangeChoice::Reload);
}

void ExternalChangeDialogTest::keepCurrentButtonMeansKeep()
{
    QCOMPARE(ask([](ExternalChangeDialog& box) { box.keepButton()->click(); }),
             ExternalChangeChoice::Keep);
}

void ExternalChangeDialogTest::escapeMeansKeep()
{
    QCOMPARE(ask([](ExternalChangeDialog& box) { QTest::keyClick(&box, Qt::Key_Escape); }),
             ExternalChangeChoice::Keep);
}

void ExternalChangeDialogTest::enterReloadsWhenThereAreNoUnsavedChanges()
{
    QCOMPARE(ask([](ExternalChangeDialog& box) {
                 QVERIFY(box.primaryButton()->isDefault());
                 QTest::keyClick(&box, Qt::Key_Return);
             }),
             ExternalChangeChoice::Reload);
}

void ExternalChangeDialogTest::enterKeepsWhenReloadWouldLoseEdits()
{
    QCOMPARE(ask(
                 [](ExternalChangeDialog& box) {
                     QVERIFY(box.keepButton()->isDefault());
                     QVERIFY(!box.primaryButton()->isDefault());
                     QTest::keyClick(&box, Qt::Key_Return);
                 },
                 ExternalChangeKind::Changed, true),
             ExternalChangeChoice::Keep);
}

void ExternalChangeDialogTest::deletedFileOffersCloseTabAndKeep()
{
    ask(
        [](ExternalChangeDialog& box) {
            QVERIFY(box.primaryButton()->text().contains(QStringLiteral("Close tab")));
            QVERIFY(box.keepButton()->text().remove(QLatin1Char('&')) == QStringLiteral("Keep"));
        },
        ExternalChangeKind::Deleted);
}

void ExternalChangeDialogTest::closeTabMeansClose()
{
    QCOMPARE(ask([](ExternalChangeDialog& box) { box.primaryButton()->click(); },
                 ExternalChangeKind::Deleted),
             ExternalChangeChoice::Close);
}

void ExternalChangeDialogTest::enterKeepsADeletedFile()
{
    QCOMPARE(ask([](ExternalChangeDialog& box) { QTest::keyClick(&box, Qt::Key_Return); },
                 ExternalChangeKind::Deleted),
             ExternalChangeChoice::Keep);
}

void ExternalChangeDialogTest::textsDescribeEachSituation()
{
    ask([](ExternalChangeDialog& box) {
        QVERIFY(box.titleLabel()->text().contains(QStringLiteral("notes.txt")));
        QVERIFY(box.titleLabel()->text().contains(QStringLiteral("changed on disk")));
        QVERIFY(box.bodyLabel()->text().contains(QStringLiteral("Reload")));
    });
    ask(
        [](ExternalChangeDialog& box) {
            QVERIFY(box.bodyLabel()->text().contains(QStringLiteral("unsaved changes")));
            QVERIFY(box.bodyLabel()->text().contains(QStringLiteral("discard")));
        },
        ExternalChangeKind::Changed, true);
    ask(
        [](ExternalChangeDialog& box) {
            QVERIFY(box.titleLabel()->text().contains(QStringLiteral("was deleted")));
        },
        ExternalChangeKind::Deleted);
}

void ExternalChangeDialogTest::longNameIsShortenedToFitOneLine()
{
    const QString longName = QString(200, QLatin1Char('x')) + QStringLiteral(".txt");
    ask(
        [&](ExternalChangeDialog& box) {
            const QLabel* title = box.titleLabel();
            QVERIFY(title->text().contains(QStringLiteral("changed on disk")));
            QVERIFY(title->text().contains(QChar(0x2026))); // the ellipsis
            QVERIFY(title->text().endsWith(QStringLiteral("changed on disk")));
            QVERIFY(title->sizeHint().width() <= ExternalChangeDialog::kCardWidth);
        },
        ExternalChangeKind::Changed, false, longName);
}

void ExternalChangeDialogTest::dialogHasNoSystemTitleBar()
{
    ask([](ExternalChangeDialog& box) {
        QVERIFY(box.windowFlags().testFlag(Qt::FramelessWindowHint));
    });
}

void ExternalChangeDialogTest::defaultDialogsAreWiredUp()
{
    QVERIFY(static_cast<bool>(defaultFileDialogs().confirmExternalChange));
}

QTEST_MAIN(ExternalChangeDialogTest)
#include "ExternalChangeDialogTest.moc"
