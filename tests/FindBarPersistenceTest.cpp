#include "FindBarPersistence.h"

#include "EditorTabs.h"
#include "FindBar.h"
#include "SettingsManager.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using scribe::core::SettingsManager;
using scribe::editor::EditorTabs;
using scribe::editor::FindBar;
using scribe::editor::FindBarPersistence;

class FindBarPersistenceTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void defaultsStayWhenNothingIsSaved();
    void loadingDoesNotCreateTheFile();
    void savedOptionsAreAppliedToTheBar();
    void changingMatchCaseIsSavedAtOnce();
    void changingWrapAroundIsSavedAtOnce();
    void bothKeysAreWrittenReadably();
    void aDamagedValueLeavesThatOptionAlone();
    void theOptionsSurviveAWholeRoundTrip();
    void changesBeforeItWasCreatedAreNotLost();

private:
    QString newPath() { return m_dir->filePath(QStringLiteral("f%1.ini").arg(++m_counter)); }
    QString write(const QByteArray& bytes)
    {
        const QString path = newPath();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return {};
        }
        file.write(bytes);
        return QString(path);
    }
    static QString read(const QString& path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    std::unique_ptr<QTemporaryDir> m_dir;
    int m_counter = 0;
};

void FindBarPersistenceTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

void FindBarPersistenceTest::defaultsStayWhenNothingIsSaved()
{
    SettingsManager settings(newPath());
    EditorTabs tabs;
    FindBar bar(tabs);
    const bool matchCase = bar.matchCase();
    const bool wrap = bar.wrapAround();
    FindBarPersistence persistence(bar, settings);
    QCOMPARE(bar.matchCase(), matchCase);
    QCOMPARE(bar.wrapAround(), wrap);
}

void FindBarPersistenceTest::loadingDoesNotCreateTheFile()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        EditorTabs tabs;
        FindBar bar(tabs);
        FindBarPersistence persistence(bar, settings);
    }
    QVERIFY(!QFile::exists(path));
}

void FindBarPersistenceTest::savedOptionsAreAppliedToTheBar()
{
    SettingsManager settings(write("[find]\nmatchCase=true\nwrapAround=false\n"));
    EditorTabs tabs;
    FindBar bar(tabs);
    FindBarPersistence persistence(bar, settings);
    QVERIFY(bar.matchCase());
    QVERIFY(!bar.wrapAround());
}

void FindBarPersistenceTest::changingMatchCaseIsSavedAtOnce()
{
    const QString path = newPath();
    SettingsManager settings(path);
    EditorTabs tabs;
    FindBar bar(tabs);
    FindBarPersistence persistence(bar, settings);

    bar.setMatchCase(!bar.matchCase());
    QCOMPARE(settings.boolValue(FindBarPersistence::kMatchCaseKey, !bar.matchCase()),
             bar.matchCase());
    QVERIFY(QFile::exists(path));
}

void FindBarPersistenceTest::changingWrapAroundIsSavedAtOnce()
{
    SettingsManager settings(newPath());
    EditorTabs tabs;
    FindBar bar(tabs);
    FindBarPersistence persistence(bar, settings);

    bar.setWrapAround(!bar.wrapAround());
    QCOMPARE(settings.boolValue(FindBarPersistence::kWrapAroundKey, !bar.wrapAround()),
             bar.wrapAround());
}

void FindBarPersistenceTest::bothKeysAreWrittenReadably()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        EditorTabs tabs;
        FindBar bar(tabs);
        FindBarPersistence persistence(bar, settings);
        bar.setMatchCase(true);
        bar.setWrapAround(false);
    }
    const QString text = read(path);
    QVERIFY2(text.contains(QStringLiteral("[find]")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("matchCase=true")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("wrapAround=false")), qPrintable(text));
}

void FindBarPersistenceTest::aDamagedValueLeavesThatOptionAlone()
{
    SettingsManager settings(write("[find]\nmatchCase=maybe\nwrapAround=false\n"));
    EditorTabs tabs;
    FindBar bar(tabs);
    const bool matchCase = bar.matchCase();
    FindBarPersistence persistence(bar, settings);
    QCOMPARE(bar.matchCase(), matchCase);
    QVERIFY(!bar.wrapAround());
}

void FindBarPersistenceTest::theOptionsSurviveAWholeRoundTrip()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        EditorTabs tabs;
        FindBar bar(tabs);
        FindBarPersistence persistence(bar, settings);
        bar.setMatchCase(true);
        bar.setWrapAround(false);
    }
    SettingsManager settings(path);
    EditorTabs tabs;
    FindBar bar(tabs);
    FindBarPersistence persistence(bar, settings);
    QVERIFY(bar.matchCase());
    QVERIFY(!bar.wrapAround());
}

void FindBarPersistenceTest::changesBeforeItWasCreatedAreNotLost()
{
    // A bar changed by code before persistence starts keeps the saved value, since the saved one
    // wins; but once an option differs from the file, the next toggle writes both options.
    SettingsManager settings(write("[find]\nmatchCase=true\n"));
    EditorTabs tabs;
    FindBar bar(tabs);
    FindBarPersistence persistence(bar, settings);
    bar.setWrapAround(!bar.wrapAround());
    QVERIFY(settings.contains(FindBarPersistence::kWrapAroundKey));
    QVERIFY(settings.boolValue(FindBarPersistence::kMatchCaseKey, false));
}

QTEST_MAIN(FindBarPersistenceTest)
#include "FindBarPersistenceTest.moc"
