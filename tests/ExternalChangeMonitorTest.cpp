#include "ExternalChangeMonitor.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest>

#include <functional>
#include <memory>

using scribe::app::ExternalChangeMonitor;
using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;

// The monitor with scripted answers instead of dialogs. Most tests call checkNow() directly; the
// ones at the end let the real file system watcher and its debounce do the work.
class ExternalChangeMonitorTest : public QObject
{
    Q_OBJECT

    struct Question
    {
        QString name;
        ExternalChangeKind kind;
        bool hasUnsavedChanges;
    };

private slots:
    void init();

    void unchangedFilesAreNotReported();
    void untitledTabsAreIgnored();
    void changedFileAsksAndReloads();
    void keepCurrentKeepsTheTextAndDoesNotAskAgain();
    void aSecondChangeAfterKeepAsksAgain();
    void modifiedTabTellsTheDialogThereAreUnsavedChanges();
    void reloadDiscardsTheEditsOfAModifiedTab();
    void ownSaveIsNotAChange();
    void ownSaveAsIsNotAChange();
    void deletedFileAsksWithKindDeleted();
    void keepingADeletedFileMakesItModifiedAndSavable();
    void keepingADeletedFileDoesNotAskAgain();
    void closeRemovesTheTabOfADeletedFile();
    void closeOfAModifiedDeletedFileCancelledByTheGuardKeepsTheTab();
    void reloadIsIgnoredForADeletedFile();
    void closeIsIgnoredForAChangedFile();
    void withoutADialogFunctionNothingHappens();
    void everyChangedTabIsAskedAboutInTabOrderAndMadeActive();
    void aTabClosedWhileAskingIsSkipped();
    void checkNowFromInsideAQuestionDoesNotNest();
    void failedReloadShowsAnErrorAndDoesNotAskAgain();
    void aFileThatChangedBackIsNotReported();

    void watcherReportsAnExternalWrite();
    void watcherStillWorksAfterTheFileWasReplaced();
    void aBurstOfWritesIsOneQuestion();
    void aFileOpenedLaterIsWatchedToo();

private:
    QString path(const QString& name) const { return m_dir->filePath(name); }
    void write(const QString& name, const QByteArray& bytes) const;
    QByteArray read(const QString& name) const;
    EditorWidget* open(const QString& name, const QByteArray& content);

    /// Rebuilds the tabs and monitor with `answer` as the user.
    void startMonitor(std::function<ExternalChangeChoice(const Question&)> answer);

    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<EditorTabs> m_tabs;
    std::unique_ptr<ExternalChangeMonitor> m_monitor;
    QList<Question> m_questions;
    QStringList m_errors;
};

void ExternalChangeMonitorTest::init()
{
    m_monitor.reset();
    m_tabs = std::make_unique<EditorTabs>();
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_questions.clear();
    m_errors.clear();
    startMonitor([](const Question&) { return ExternalChangeChoice::Keep; });
}

void ExternalChangeMonitorTest::startMonitor(
    std::function<ExternalChangeChoice(const Question&)> answer)
{
    m_monitor.reset();
    FileDialogs dialogs;
    dialogs.confirmExternalChange = [this, answer](QWidget*, const QString& name,
                                                   ExternalChangeKind kind, bool unsaved) {
        const Question question{name, kind, unsaved};
        m_questions.append(question);
        return answer(question);
    };
    dialogs.showError = [this](QWidget*, const QString&, const QString& message) {
        m_errors.append(message);
    };
    m_monitor = std::make_unique<ExternalChangeMonitor>(*m_tabs, dialogs, nullptr);
}

void ExternalChangeMonitorTest::write(const QString& name, const QByteArray& bytes) const
{
    QFile file(path(name));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(bytes);
}

QByteArray ExternalChangeMonitorTest::read(const QString& name) const
{
    QFile file(path(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

EditorWidget* ExternalChangeMonitorTest::open(const QString& name, const QByteArray& content)
{
    write(name, content);
    if (!m_tabs->openFile(path(name)).ok()) {
        return nullptr;
    }
    return m_tabs->currentEditor();
}

void ExternalChangeMonitorTest::unchangedFilesAreNotReported()
{
    QVERIFY(open("a.txt", "one") != nullptr);
    m_monitor->checkNow();
    m_monitor->checkNow();
    QVERIFY(m_questions.isEmpty());
}

void ExternalChangeMonitorTest::untitledTabsAreIgnored()
{
    m_tabs->currentEditor()->insertPlainText(QStringLiteral("scratch"));
    m_monitor->checkNow();
    QVERIFY(m_questions.isEmpty());
}

void ExternalChangeMonitorTest::changedFileAsksAndReloads()
{
    startMonitor([](const Question&) { return ExternalChangeChoice::Reload; });
    EditorWidget* editor = open("a.txt", "one");
    write("a.txt", "one and more");

    m_monitor->checkNow();

    QCOMPARE(m_questions.size(), 1);
    QCOMPARE(m_questions[0].name, QStringLiteral("a.txt"));
    QCOMPARE(m_questions[0].kind, ExternalChangeKind::Changed);
    QVERIFY(!m_questions[0].hasUnsavedChanges);
    QCOMPARE(editor->toPlainText(), QStringLiteral("one and more"));
    QVERIFY(!editor->isModified());

    m_monitor->checkNow(); // reloaded, so it is the known state now
    QCOMPARE(m_questions.size(), 1);
}

void ExternalChangeMonitorTest::keepCurrentKeepsTheTextAndDoesNotAskAgain()
{
    EditorWidget* editor = open("a.txt", "one");
    write("a.txt", "one and more");

    m_monitor->checkNow();
    m_monitor->checkNow();
    m_monitor->checkNow();

    QCOMPARE(m_questions.size(), 1);
    QCOMPARE(editor->toPlainText(), QStringLiteral("one"));
}

void ExternalChangeMonitorTest::aSecondChangeAfterKeepAsksAgain()
{
    open("a.txt", "one");
    write("a.txt", "one and more");
    m_monitor->checkNow();
    write("a.txt", "one and a lot more");
    m_monitor->checkNow();
    QCOMPARE(m_questions.size(), 2);
}

void ExternalChangeMonitorTest::modifiedTabTellsTheDialogThereAreUnsavedChanges()
{
    EditorWidget* editor = open("a.txt", "one");
    editor->insertPlainText(QStringLiteral("mine "));
    write("a.txt", "one and more");

    m_monitor->checkNow();

    QCOMPARE(m_questions.size(), 1);
    QVERIFY(m_questions[0].hasUnsavedChanges);
    QCOMPARE(editor->toPlainText(), QStringLiteral("mine one")); // Keep: nothing lost
    QVERIFY(editor->isModified());
}

void ExternalChangeMonitorTest::reloadDiscardsTheEditsOfAModifiedTab()
{
    startMonitor([](const Question&) { return ExternalChangeChoice::Reload; });
    EditorWidget* editor = open("a.txt", "one");
    editor->insertPlainText(QStringLiteral("mine "));
    write("a.txt", "one and more");

    m_monitor->checkNow();

    QCOMPARE(editor->toPlainText(), QStringLiteral("one and more"));
    QVERIFY(!editor->isModified());
}

void ExternalChangeMonitorTest::ownSaveIsNotAChange()
{
    EditorWidget* editor = open("a.txt", "one");
    editor->insertPlainText(QStringLiteral("a much longer text "));
    QVERIFY(editor->saveFile().ok());

    m_monitor->checkNow();
    QVERIFY(m_questions.isEmpty());
}

void ExternalChangeMonitorTest::ownSaveAsIsNotAChange()
{
    EditorWidget* editor = open("a.txt", "one");
    QVERIFY(editor->saveFileAs(path("b.txt")).ok());

    m_monitor->checkNow();
    QVERIFY(m_questions.isEmpty());

    write("b.txt", "changed by someone else");
    m_monitor->checkNow();
    QCOMPARE(m_questions.size(), 1);
    QCOMPARE(m_questions[0].name, QStringLiteral("b.txt")); // now tracks the new path
}

void ExternalChangeMonitorTest::deletedFileAsksWithKindDeleted()
{
    open("a.txt", "one");
    QVERIFY(QFile::remove(path("a.txt")));

    m_monitor->checkNow();

    QCOMPARE(m_questions.size(), 1);
    QCOMPARE(m_questions[0].kind, ExternalChangeKind::Deleted);
}

void ExternalChangeMonitorTest::keepingADeletedFileMakesItModifiedAndSavable()
{
    EditorWidget* editor = open("a.txt", "one");
    QVERIFY(QFile::remove(path("a.txt")));

    m_monitor->checkNow();

    QVERIFY(editor->isModified());
    QCOMPARE(editor->toPlainText(), QStringLiteral("one"));
    QVERIFY(editor->saveFile().ok());
    QCOMPARE(read("a.txt"), QByteArray("one"));
    m_monitor->checkNow();
    QCOMPARE(m_questions.size(), 1); // our own save recreated it: not a change
}

void ExternalChangeMonitorTest::keepingADeletedFileDoesNotAskAgain()
{
    open("a.txt", "one");
    QVERIFY(QFile::remove(path("a.txt")));
    m_monitor->checkNow();
    m_monitor->checkNow();
    QCOMPARE(m_questions.size(), 1);
}

void ExternalChangeMonitorTest::closeRemovesTheTabOfADeletedFile()
{
    startMonitor([](const Question&) { return ExternalChangeChoice::Close; });
    open("a.txt", "one");
    open("b.txt", "two");
    QCOMPARE(m_tabs->count(), 2);
    QVERIFY(QFile::remove(path("b.txt")));

    m_monitor->checkNow();

    QCOMPARE(m_tabs->count(), 1);
    QCOMPARE(m_tabs->currentEditor()->displayName(), QStringLiteral("a.txt"));
}

void ExternalChangeMonitorTest::closeOfAModifiedDeletedFileCancelledByTheGuardKeepsTheTab()
{
    startMonitor([](const Question&) { return ExternalChangeChoice::Close; });
    m_tabs->setCloseGuard([](EditorWidget&) { return false; }); // the user cancels "Save changes?"
    EditorWidget* editor = open("a.txt", "one");
    editor->insertPlainText(QStringLiteral("mine "));
    QVERIFY(QFile::remove(path("a.txt")));

    m_monitor->checkNow();
    m_monitor->checkNow();

    QCOMPARE(m_tabs->count(), 1);
    QCOMPARE(editor->toPlainText(), QStringLiteral("mine one"));
    QCOMPARE(m_questions.size(), 1); // and it is not asked about again
}

void ExternalChangeMonitorTest::reloadIsIgnoredForADeletedFile()
{
    startMonitor([](const Question&) { return ExternalChangeChoice::Reload; });
    EditorWidget* editor = open("a.txt", "one");
    QVERIFY(QFile::remove(path("a.txt")));

    m_monitor->checkNow();

    QCOMPARE(editor->toPlainText(), QStringLiteral("one")); // treated as Keep
    QVERIFY(m_errors.isEmpty());
}

void ExternalChangeMonitorTest::closeIsIgnoredForAChangedFile()
{
    startMonitor([](const Question&) { return ExternalChangeChoice::Close; });
    open("a.txt", "one");
    write("a.txt", "one and more");

    m_monitor->checkNow();

    QCOMPARE(m_tabs->count(), 1);
    QCOMPARE(m_tabs->currentEditor()->toPlainText(), QStringLiteral("one"));
}

void ExternalChangeMonitorTest::withoutADialogFunctionNothingHappens()
{
    m_monitor.reset();
    FileDialogs none;
    m_monitor = std::make_unique<ExternalChangeMonitor>(*m_tabs, none, nullptr);
    EditorWidget* editor = open("a.txt", "one");
    write("a.txt", "one and more");

    m_monitor->checkNow();

    QCOMPARE(editor->toPlainText(), QStringLiteral("one"));
    QVERIFY(!editor->isModified());
}

void ExternalChangeMonitorTest::everyChangedTabIsAskedAboutInTabOrderAndMadeActive()
{
    QStringList activeWhenAsked;
    startMonitor([&](const Question&) {
        activeWhenAsked.append(m_tabs->currentEditor()->displayName());
        return ExternalChangeChoice::Keep;
    });
    open("a.txt", "one");
    open("b.txt", "two");
    open("c.txt", "three");
    write("c.txt", "three!");
    write("a.txt", "one!");

    m_monitor->checkNow();

    QCOMPARE(activeWhenAsked, (QStringList{"a.txt", "c.txt"}));
    QCOMPARE(m_tabs->currentEditor()->displayName(), QStringLiteral("c.txt"));
}

void ExternalChangeMonitorTest::aTabClosedWhileAskingIsSkipped()
{
    int asked = 0;
    startMonitor([&](const Question&) {
        ++asked;
        m_tabs->closeTab(1); // the user (or a script) closes the other changed tab meanwhile
        return ExternalChangeChoice::Keep;
    });
    open("a.txt", "one");
    open("b.txt", "two");
    write("a.txt", "one!");
    write("b.txt", "two!");

    m_monitor->checkNow(); // must not crash on the closed tab

    QCOMPARE(asked, 1);
    QCOMPARE(m_tabs->count(), 1);
}

void ExternalChangeMonitorTest::checkNowFromInsideAQuestionDoesNotNest()
{
    int asked = 0;
    bool askingInside = false;
    startMonitor([&](const Question&) {
        ++asked;
        askingInside = m_monitor->isAsking();
        m_monitor->checkNow(); // for example the window was activated again behind the dialog
        return ExternalChangeChoice::Keep;
    });
    open("a.txt", "one");
    write("a.txt", "one!");

    m_monitor->checkNow();

    QCOMPARE(asked, 1);
    QVERIFY(askingInside);
    QVERIFY(!m_monitor->isAsking());
}

void ExternalChangeMonitorTest::failedReloadShowsAnErrorAndDoesNotAskAgain()
{
    startMonitor([this](const Question&) {
        QFile::remove(path("a.txt")); // the file vanishes while the question is open
        return ExternalChangeChoice::Reload;
    });
    EditorWidget* editor = open("a.txt", "one");
    write("a.txt", "one!");

    m_monitor->checkNow();

    QCOMPARE(m_errors.size(), 1);
    QCOMPARE(editor->toPlainText(), QStringLiteral("one")); // the content survives
    m_monitor->checkNow();
    QCOMPARE(m_questions.size(), 1);
}

void ExternalChangeMonitorTest::aFileThatChangedBackIsNotReported()
{
    open("a.txt", "one");
    const QByteArray original = read("a.txt");
    const QFileInfo before(path("a.txt"));
    const QDateTime time = before.lastModified();

    write("a.txt", "different");
    write("a.txt", original);
    QFile file(path("a.txt"));
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.setFileTime(time, QFileDevice::FileModificationTime));
    file.close();

    m_monitor->checkNow();
    QVERIFY(m_questions.isEmpty());
}

void ExternalChangeMonitorTest::watcherReportsAnExternalWrite()
{
    EditorWidget* editor = open("a.txt", "one");
    QVERIFY(editor != nullptr);

    write("a.txt", "one and more"); // no checkNow(): the watcher has to notice

    QTRY_COMPARE_WITH_TIMEOUT(m_questions.size(), 1, 10000);
    QCOMPARE(m_questions[0].kind, ExternalChangeKind::Changed);
}

void ExternalChangeMonitorTest::watcherStillWorksAfterTheFileWasReplaced()
{
    open("a.txt", "one");

    // Many programs save by writing a temporary file and renaming it over the original; that ends
    // the watch on the file itself.
    {
        QSaveFile replacement(path("a.txt"));
        QVERIFY(replacement.open(QIODevice::WriteOnly));
        replacement.write("replaced once");
        QVERIFY(replacement.commit());
    }
    QTRY_COMPARE_WITH_TIMEOUT(m_questions.size(), 1, 10000);

    {
        QSaveFile replacement(path("a.txt"));
        QVERIFY(replacement.open(QIODevice::WriteOnly));
        replacement.write("replaced a second time");
        QVERIFY(replacement.commit());
    }
    QTRY_COMPARE_WITH_TIMEOUT(m_questions.size(), 2, 10000);
}

void ExternalChangeMonitorTest::aBurstOfWritesIsOneQuestion()
{
    open("a.txt", "one");

    for (int i = 1; i <= 5; ++i) {
        write("a.txt", QByteArray(i * 10, 'x'));
        QTest::qWait(30); // well inside the debounce time
    }

    QTRY_COMPARE_WITH_TIMEOUT(m_questions.size(), 1, 10000);
    QTest::qWait(ExternalChangeMonitor::kDebounceMs * 2);
    QCOMPARE(m_questions.size(), 1);
}

void ExternalChangeMonitorTest::aFileOpenedLaterIsWatchedToo()
{
    open("a.txt", "one");
    open("b.txt", "two"); // opened after the monitor started

    write("b.txt", "two and more");

    QTRY_COMPARE_WITH_TIMEOUT(m_questions.size(), 1, 10000);
    QCOMPARE(m_questions[0].name, QStringLiteral("b.txt"));
}

QTEST_MAIN(ExternalChangeMonitorTest)
#include "ExternalChangeMonitorTest.moc"
