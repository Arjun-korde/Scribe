#include "SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QThread>

#include <gtest/gtest.h>

using scribe::core::SettingsManager;

namespace
{

QString s(const char* text)
{
    return QString::fromUtf8(text);
}

} // namespace

// Every test works on a file in its own temporary folder, never on the user's real settings.
class SettingsManagerTest : public testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(m_dir.isValid());
        // Tests on purpose make the file unwritable; keep the expected warning out of the output.
        QLoggingCategory::setFilterRules(QStringLiteral("scribe.settings.warning=false"));
    }
    void TearDown() override { QLoggingCategory::setFilterRules(QString()); }

    QString path(const QString& name = QStringLiteral("scribe.ini")) const
    {
        return m_dir.filePath(name);
    }

    static QByteArray readFile(const QString& filePath)
    {
        QFile file(filePath);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    static void writeFile(const QString& filePath, const QByteArray& bytes)
    {
        QFile file(filePath);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(bytes);
    }

    QTemporaryDir m_dir;
};

// ---- Defaults and not touching the disk -----------------------------------------------------

TEST_F(SettingsManagerTest, ReturnsTheDefaultForEverythingWhenThereIsNoFile)
{
    const SettingsManager settings(path());

    EXPECT_TRUE(settings.boolValue(QStringLiteral("view/wordWrap"), true));
    EXPECT_FALSE(settings.boolValue(QStringLiteral("view/wordWrap"), false));
    EXPECT_EQ(settings.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 100);
    EXPECT_EQ(settings.stringValue(QStringLiteral("view/font"), s("Consolas")), s("Consolas"));
    EXPECT_TRUE(settings.stringList(QStringLiteral("files/recent")).isEmpty());
    EXPECT_FALSE(settings.contains(QStringLiteral("view/zoom")));
    EXPECT_EQ(settings.version(), 0);
}

TEST_F(SettingsManagerTest, ReadingAndOpeningNeverCreatesTheFile)
{
    {
        const SettingsManager settings(path());
        (void)settings.boolValue(QStringLiteral("a/b"), true);
        (void)settings.stringList(QStringLiteral("c/d"));
    }
    EXPECT_FALSE(QFileInfo::exists(path()));
}

TEST_F(SettingsManagerTest, KeepsTheFilePathItWasGiven)
{
    const SettingsManager settings(path());
    EXPECT_EQ(settings.filePath(), path());
}

TEST_F(SettingsManagerTest, StartsHealthy)
{
    const SettingsManager settings(path());
    EXPECT_TRUE(settings.isHealthy());
}

// ---- Round trips ----------------------------------------------------------------------------

TEST_F(SettingsManagerTest, SavedValuesSurviveAFreshInstance)
{
    {
        SettingsManager settings(path());
        settings.setBool(QStringLiteral("view/wordWrap"), false);
        settings.setInt(QStringLiteral("view/zoom"), 150);
        settings.setString(QStringLiteral("view/fontFamily"), s("Cascadia Mono"));
        settings.setStringList(QStringLiteral("files/recent"), {s("a.txt"), s("b.txt")});
    }

    const SettingsManager again(path());
    EXPECT_FALSE(again.boolValue(QStringLiteral("view/wordWrap"), true));
    EXPECT_EQ(again.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 150);
    EXPECT_EQ(again.stringValue(QStringLiteral("view/fontFamily"), QString()), s("Cascadia Mono"));
    EXPECT_EQ(again.stringList(QStringLiteral("files/recent")),
              (QStringList{s("a.txt"), s("b.txt")}));
}

TEST_F(SettingsManagerTest, ChangesAreOnDiskImmediatelyNotOnlyWhenClosing)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("view/zoom"), 170);

    // Look at the file itself while the instance is still alive, as a crash right now would leave
    // it. (Another SettingsManager would not prove this: Qt shares parsed files inside a process.)
    EXPECT_TRUE(QString::fromUtf8(readFile(path())).contains(QStringLiteral("zoom=170")));
}

TEST_F(SettingsManagerTest, TheLastValueWins)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("view/zoom"), 120);
    settings.setInt(QStringLiteral("view/zoom"), 130);
    settings.setBool(QStringLiteral("view/wordWrap"), true);
    settings.setBool(QStringLiteral("view/wordWrap"), false);

    const SettingsManager again(path());
    EXPECT_EQ(again.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 130);
    EXPECT_FALSE(again.boolValue(QStringLiteral("view/wordWrap"), true));
}

TEST_F(SettingsManagerTest, NegativeAndZeroNumbersRoundTrip)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("a/negative"), -5);
    settings.setInt(QStringLiteral("a/zero"), 0);

    const SettingsManager again(path());
    EXPECT_EQ(again.intValue(QStringLiteral("a/negative"), 99, -10, 10), -5);
    EXPECT_EQ(again.intValue(QStringLiteral("a/zero"), 99, -10, 10), 0);
}

// ---- The file is plain, hand-editable INI ---------------------------------------------------

TEST_F(SettingsManagerTest, WritesAReadableIniFileWithSections)
{
    {
        SettingsManager settings(path());
        settings.setBool(QStringLiteral("view/wordWrap"), true);
        settings.setInt(QStringLiteral("view/zoom"), 120);
    }

    const QString text = QString::fromUtf8(readFile(path()));
    EXPECT_TRUE(text.contains(QStringLiteral("[view]"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("wordWrap=true"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("zoom=120"))) << qPrintable(text);
}

TEST_F(SettingsManagerTest, ReadsAFileThatWasEditedByHand)
{
    writeFile(path(), "[view]\nwordWrap=false\nzoom=150\nfontFamily=Courier New\n");

    const SettingsManager settings(path());
    EXPECT_FALSE(settings.boolValue(QStringLiteral("view/wordWrap"), true));
    EXPECT_EQ(settings.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 150);
    EXPECT_EQ(settings.stringValue(QStringLiteral("view/fontFamily"), QString()), s("Courier New"));
}

TEST_F(SettingsManagerTest, AHandTypedValueWithCommasKeepsItsWords)
{
    // Qt reads an unquoted value with commas as a list and trims the spaces after the commas, so
    // the exact spacing of a hand-typed value cannot be recovered. The words and commas are kept,
    // and the value is still one string rather than being lost. (Values written by the program
    // itself are quoted and round trip exactly, see StringsWithSpecialCharactersRoundTripExactly.)
    writeFile(path(), "[view]\nfontFamily=Courier New, Bold\n");

    const SettingsManager settings(path());
    EXPECT_EQ(settings.stringValue(QStringLiteral("view/fontFamily"), QString()),
              s("Courier New,Bold"));
}

TEST_F(SettingsManagerTest, AcceptsTheCommonSpellingsOfABool)
{
    writeFile(path(), "[t]\na=TRUE\nb=False\nc=1\nd=0\ne= true \n");

    const SettingsManager settings(path());
    EXPECT_TRUE(settings.boolValue(QStringLiteral("t/a"), false));
    EXPECT_FALSE(settings.boolValue(QStringLiteral("t/b"), true));
    EXPECT_TRUE(settings.boolValue(QStringLiteral("t/c"), false));
    EXPECT_FALSE(settings.boolValue(QStringLiteral("t/d"), true));
    EXPECT_TRUE(settings.boolValue(QStringLiteral("t/e"), false));
}

// ---- Damaged or odd values never break anything ---------------------------------------------

TEST_F(SettingsManagerTest, AnInvalidBoolGivesTheDefault)
{
    writeFile(path(), "[t]\na=banana\nb=\nc=2\nd=yes\n");

    const SettingsManager settings(path());
    for (const char* key : {"t/a", "t/b", "t/c", "t/d"}) {
        EXPECT_TRUE(settings.boolValue(QLatin1String(key), true)) << key;
        EXPECT_FALSE(settings.boolValue(QLatin1String(key), false)) << key;
    }
}

TEST_F(SettingsManagerTest, AnInvalidNumberGivesTheDefault)
{
    writeFile(path(), "[t]\na=banana\nb=\nc=5.5\nd=12abc\ne=99999999999999999999\nf=0x10\ng=1e2\n");

    const SettingsManager settings(path());
    for (const char* key : {"t/a", "t/b", "t/c", "t/d", "t/e", "t/f", "t/g"}) {
        EXPECT_EQ(settings.intValue(QLatin1String(key), 42, 0, 1000), 42) << key;
    }
}

TEST_F(SettingsManagerTest, ANumberOutsideTheRangeGivesTheDefault)
{
    writeFile(path(), "[t]\nlow=5\nhigh=600\nnegative=-1\nok=100\n");

    const SettingsManager settings(path());
    EXPECT_EQ(settings.intValue(QStringLiteral("t/low"), 100, 10, 500), 100);
    EXPECT_EQ(settings.intValue(QStringLiteral("t/high"), 100, 10, 500), 100);
    EXPECT_EQ(settings.intValue(QStringLiteral("t/negative"), 100, 10, 500), 100);
    EXPECT_EQ(settings.intValue(QStringLiteral("t/ok"), 7, 10, 500), 100);
}

TEST_F(SettingsManagerTest, TheRangeLimitsThemselvesAreAllowed)
{
    writeFile(path(), "[t]\nmin=10\nmax=500\n");

    const SettingsManager settings(path());
    EXPECT_EQ(settings.intValue(QStringLiteral("t/min"), 100, 10, 500), 10);
    EXPECT_EQ(settings.intValue(QStringLiteral("t/max"), 100, 10, 500), 500);
}

TEST_F(SettingsManagerTest, ASurroundingSpaceInANumberIsIgnored)
{
    writeFile(path(), "[t]\na= 120 \n");
    const SettingsManager settings(path());
    EXPECT_EQ(settings.intValue(QStringLiteral("t/a"), 100, 10, 500), 120);
}

TEST_F(SettingsManagerTest, AFileOfGarbageGivesDefaultsAndDoesNotCrash)
{
    QByteArray garbage;
    for (int i = 0; i < 2000; ++i) {
        garbage.append(static_cast<char>((i * 37 + 11) % 256));
    }
    writeFile(path(), garbage);

    SettingsManager settings(path());
    EXPECT_TRUE(settings.boolValue(QStringLiteral("view/wordWrap"), true));
    EXPECT_EQ(settings.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 100);
    EXPECT_TRUE(settings.stringList(QStringLiteral("files/recent")).isEmpty());

    // And it can be used and saved afterwards.
    settings.setInt(QStringLiteral("view/zoom"), 130);
    const SettingsManager again(path());
    EXPECT_EQ(again.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 130);
}

TEST_F(SettingsManagerTest, AnEmptyFileIsJustNoSettings)
{
    writeFile(path(), "");
    const SettingsManager settings(path());
    EXPECT_EQ(settings.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 100);
    EXPECT_TRUE(settings.isHealthy());
}

TEST_F(SettingsManagerTest, AMissingKeyDoesNotDisturbTheOthers)
{
    writeFile(path(), "[view]\nzoom=140\n");
    const SettingsManager settings(path());
    EXPECT_EQ(settings.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 140);
    EXPECT_TRUE(settings.boolValue(QStringLiteral("view/wordWrap"), true)); // not in the file
}

TEST_F(SettingsManagerTest, AnEmptyKeyIsIgnored)
{
    SettingsManager settings(path());
    settings.setInt(QString(), 5);
    settings.setBool(QString(), true);
    settings.setString(QString(), s("x"));
    settings.setStringList(QString(), {s("x")});
    settings.remove(QString());

    EXPECT_EQ(settings.intValue(QString(), 9, 0, 10), 9);
    EXPECT_FALSE(settings.contains(QString()));
    EXPECT_FALSE(QFileInfo::exists(path())); // nothing was written for an empty key
}

// ---- Strings with special characters --------------------------------------------------------

TEST_F(SettingsManagerTest, StringsWithSpecialCharactersRoundTripExactly)
{
    const QStringList tricky = {
        s("C:\\Users\\Me\\My Documents\\a,b.txt"),
        s("quote \" and ' inside"),
        s("  leading and trailing spaces  "),
        s("caf\xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80"),
        s("semicolon; hash # equals = brackets [x]"),
        s("two\nlines"),
        s("tab\there"),
        s("100% done"),
        s("back\\slash at end\\"),
        QString(),
    };

    {
        SettingsManager settings(path());
        for (int i = 0; i < tricky.size(); ++i) {
            settings.setString(QStringLiteral("strings/s%1").arg(i), tricky.at(i));
        }
    }

    const SettingsManager again(path());
    for (int i = 0; i < tricky.size(); ++i) {
        EXPECT_EQ(again.stringValue(QStringLiteral("strings/s%1").arg(i), s("DEFAULT")),
                  tricky.at(i))
            << "string " << i;
    }
}

TEST_F(SettingsManagerTest, AnEmptyStringIsDifferentFromAMissingOne)
{
    SettingsManager settings(path());
    settings.setString(QStringLiteral("a/empty"), QString());

    const SettingsManager again(path());
    EXPECT_TRUE(again.contains(QStringLiteral("a/empty")));
    EXPECT_EQ(again.stringValue(QStringLiteral("a/empty"), s("DEFAULT")), QString());
    EXPECT_EQ(again.stringValue(QStringLiteral("a/missing"), s("DEFAULT")), s("DEFAULT"));
}

// ---- Lists ----------------------------------------------------------------------------------

TEST_F(SettingsManagerTest, AListKeepsItsOrderAndItsDuplicates)
{
    {
        SettingsManager settings(path());
        settings.setStringList(QStringLiteral("files/recent"), {s("c"), s("a"), s("b"), s("a")});
    }
    const SettingsManager again(path());
    EXPECT_EQ(again.stringList(QStringLiteral("files/recent")),
              (QStringList{s("c"), s("a"), s("b"), s("a")}));
}

TEST_F(SettingsManagerTest, ListEntriesWithCommasAndOtherSpecialCharactersAreSafe)
{
    const QStringList paths = {
        s("C:\\a,b\\c,d.txt"),
        s("D:\\x y\\\"quoted\".txt"),
        s("E:\\caf\xC3\xA9\\\xF0\x9F\x98\x80.txt"),
        s("  spaces  "),
        QString(),
        s("one\ntwo"),
    };
    {
        SettingsManager settings(path());
        settings.setStringList(QStringLiteral("files/recent"), paths);
    }
    const SettingsManager again(path());
    EXPECT_EQ(again.stringList(QStringLiteral("files/recent")), paths);
}

TEST_F(SettingsManagerTest, ReplacingAListWithAShorterOneLeavesNothingBehind)
{
    SettingsManager settings(path());
    settings.setStringList(QStringLiteral("files/recent"),
                           {s("one"), s("two"), s("three"), s("four"), s("five")});
    settings.setStringList(QStringLiteral("files/recent"), {s("only"), s("two")});

    const SettingsManager again(path());
    EXPECT_EQ(again.stringList(QStringLiteral("files/recent")), (QStringList{s("only"), s("two")}));
    const QString text = QString::fromUtf8(readFile(path()));
    EXPECT_FALSE(text.contains(QStringLiteral("three"))) << qPrintable(text);
    EXPECT_FALSE(text.contains(QStringLiteral("five"))) << qPrintable(text);
}

TEST_F(SettingsManagerTest, AnEmptyListRemovesTheKey)
{
    SettingsManager settings(path());
    settings.setStringList(QStringLiteral("files/recent"), {s("a"), s("b")});
    settings.setStringList(QStringLiteral("files/recent"), {});

    const SettingsManager again(path());
    EXPECT_TRUE(again.stringList(QStringLiteral("files/recent")).isEmpty());
    EXPECT_FALSE(QString::fromUtf8(readFile(path())).contains(QStringLiteral("a")));
}

TEST_F(SettingsManagerTest, ReadingAListStopsAtTheLimit)
{
    SettingsManager settings(path());
    QStringList many;
    for (int i = 0; i < 50; ++i) {
        many << QString::number(i);
    }
    settings.setStringList(QStringLiteral("files/recent"), many);

    EXPECT_EQ(settings.stringList(QStringLiteral("files/recent"), 5).size(), 5);
    EXPECT_EQ(settings.stringList(QStringLiteral("files/recent"), 5).first(), s("0"));
    EXPECT_EQ(settings.stringList(QStringLiteral("files/recent"), 1000).size(), 50);
    EXPECT_TRUE(settings.stringList(QStringLiteral("files/recent"), 0).isEmpty());
}

TEST_F(SettingsManagerTest, ADamagedListSizeCannotMakeReadingSlowOrHuge)
{
    writeFile(path(), "[files]\nrecent\\size=999999999\nrecent\\1\\value=a\nrecent\\2\\value=b\n");

    const SettingsManager settings(path());
    const QStringList items = settings.stringList(QStringLiteral("files/recent"), 10);

    EXPECT_EQ(items.size(), 10); // cut at the limit, not 999999999
    EXPECT_EQ(items.first(), s("a"));
}

TEST_F(SettingsManagerTest, ListsAndSingleValuesDoNotInterfere)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("files/count"), 3);
    settings.setStringList(QStringLiteral("files/recent"), {s("a")});
    settings.setBool(QStringLiteral("files/flag"), true);

    const SettingsManager again(path());
    EXPECT_EQ(again.intValue(QStringLiteral("files/count"), 0, 0, 10), 3);
    EXPECT_EQ(again.stringList(QStringLiteral("files/recent")), (QStringList{s("a")}));
    EXPECT_TRUE(again.boolValue(QStringLiteral("files/flag"), false));
}

// ---- Removing -------------------------------------------------------------------------------

TEST_F(SettingsManagerTest, RemoveForgetsAValueAndGoesBackToTheDefault)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("view/zoom"), 150);
    settings.setInt(QStringLiteral("view/other"), 7);

    settings.remove(QStringLiteral("view/zoom"));

    const SettingsManager again(path());
    EXPECT_FALSE(again.contains(QStringLiteral("view/zoom")));
    EXPECT_EQ(again.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 100);
    EXPECT_EQ(again.intValue(QStringLiteral("view/other"), 0, 0, 10), 7); // others are kept
}

TEST_F(SettingsManagerTest, RemoveAlsoRemovesAWholeList)
{
    SettingsManager settings(path());
    settings.setStringList(QStringLiteral("files/recent"), {s("a"), s("b")});

    settings.remove(QStringLiteral("files/recent"));

    const SettingsManager again(path());
    EXPECT_TRUE(again.stringList(QStringLiteral("files/recent")).isEmpty());
}

TEST_F(SettingsManagerTest, RemovingSomethingThatIsNotThereIsHarmless)
{
    SettingsManager settings(path());
    settings.remove(QStringLiteral("never/existed"));
    EXPECT_TRUE(settings.isHealthy());
}

// ---- Version --------------------------------------------------------------------------------

TEST_F(SettingsManagerTest, TheFirstWriteRecordsTheFileVersion)
{
    SettingsManager settings(path());
    EXPECT_EQ(settings.version(), 0);

    settings.setBool(QStringLiteral("view/wordWrap"), true);

    EXPECT_EQ(settings.version(), SettingsManager::kCurrentVersion);
    EXPECT_TRUE(QString::fromUtf8(readFile(path())).contains(QStringLiteral("version=1")));
}

TEST_F(SettingsManagerTest, AFileFromANewerVersionKeepsItsVersionNumber)
{
    writeFile(path(), "[settings]\nversion=7\n[view]\nzoom=120\n");

    SettingsManager settings(path());
    EXPECT_EQ(settings.version(), 7);
    settings.setInt(QStringLiteral("view/zoom"), 130);

    const SettingsManager again(path());
    EXPECT_EQ(again.version(), 7); // not downgraded by an older program
    EXPECT_EQ(again.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 130);
}

TEST_F(SettingsManagerTest, ANonsenseVersionCountsAsNoVersion)
{
    writeFile(path(), "[settings]\nversion=banana\n");
    const SettingsManager settings(path());
    EXPECT_EQ(settings.version(), 0);
}

// ---- Not rewriting the file needlessly ------------------------------------------------------

TEST_F(SettingsManagerTest, SettingTheSameValueAgainDoesNotTouchTheFile)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("view/zoom"), 120);
    settings.setStringList(QStringLiteral("files/recent"), {s("a"), s("b")});
    const QDateTime before = QFileInfo(path()).lastModified();

    QThread::msleep(80); // longer than the file system's timestamp granularity
    settings.setInt(QStringLiteral("view/zoom"), 120);
    settings.setStringList(QStringLiteral("files/recent"), {s("a"), s("b")});

    EXPECT_EQ(QFileInfo(path()).lastModified(), before);
}

TEST_F(SettingsManagerTest, ChangingAValueDoesWriteTheFile)
{
    SettingsManager settings(path());
    settings.setInt(QStringLiteral("view/zoom"), 120);
    const QDateTime before = QFileInfo(path()).lastModified();

    QThread::msleep(80);
    settings.setInt(QStringLiteral("view/zoom"), 130);

    EXPECT_GT(QFileInfo(path()).lastModified(), before);
}

// ---- Creating the file and its folder -------------------------------------------------------

TEST_F(SettingsManagerTest, CreatesMissingFoldersOnTheFirstWrite)
{
    const QString nested = path(QStringLiteral("one/two/three/scribe.ini"));
    ASSERT_FALSE(QFileInfo::exists(QFileInfo(nested).absolutePath()));

    SettingsManager settings(nested);
    settings.setBool(QStringLiteral("view/wordWrap"), true);

    EXPECT_TRUE(QFileInfo::exists(nested));
    EXPECT_TRUE(settings.isHealthy());
    const SettingsManager again(nested);
    EXPECT_TRUE(again.boolValue(QStringLiteral("view/wordWrap"), false));
}

// ---- A location that cannot be written ------------------------------------------------------

TEST_F(SettingsManagerTest, AnUnwritableLocationDoesNotCrashAndIsReported)
{
    // The "folder" is really a file, so neither it nor the settings file can be created.
    writeFile(path(QStringLiteral("blocker")), "I am a file");
    const QString impossible = path(QStringLiteral("blocker/sub/scribe.ini"));

    SettingsManager settings(impossible);
    settings.setBool(QStringLiteral("view/wordWrap"), false);
    settings.setInt(QStringLiteral("view/zoom"), 150);
    settings.setStringList(QStringLiteral("files/recent"), {s("a")});
    settings.remove(QStringLiteral("view/zoom"));

    EXPECT_FALSE(settings.isHealthy());
    EXPECT_FALSE(QFileInfo::exists(impossible)); // nothing reached the disk

    // The program carries on: during this run the changes are still remembered in memory.
    EXPECT_FALSE(settings.boolValue(QStringLiteral("view/wordWrap"), true));
    EXPECT_EQ(settings.stringList(QStringLiteral("files/recent")), (QStringList{s("a")}));
}

TEST_F(SettingsManagerTest, AFailedWriteDoesNotStopLaterOnesFromWorkingElsewhere)
{
    writeFile(path(QStringLiteral("blocker")), "I am a file");
    SettingsManager broken(path(QStringLiteral("blocker/scribe.ini")));
    broken.setInt(QStringLiteral("view/zoom"), 150);
    EXPECT_FALSE(broken.isHealthy());

    SettingsManager fine(path());
    fine.setInt(QStringLiteral("view/zoom"), 160);
    EXPECT_TRUE(fine.isHealthy());
    const SettingsManager again(path());
    EXPECT_EQ(again.intValue(QStringLiteral("view/zoom"), 100, 10, 500), 160);
}

// ---- Two programs using the same file -------------------------------------------------------

TEST_F(SettingsManagerTest, TwoInstancesWritingDifferentKeysBothKeepTheirs)
{
    // Neither instance may wipe out the other's changes. (Inside one process Qt shares the parsed
    // file between instances; merging with another running copy of Scribe is Qt's own file
    // locking and is not simulated here.)
    SettingsManager first(path());
    SettingsManager second(path());

    first.setInt(QStringLiteral("view/zoom"), 120);
    second.setBool(QStringLiteral("view/wordWrap"), false);

    const QString text = QString::fromUtf8(readFile(path()));
    EXPECT_TRUE(text.contains(QStringLiteral("zoom=120"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("wordWrap=false"))) << qPrintable(text);
}

// ---- The default location -------------------------------------------------------------------

namespace
{

// Sets an environment variable for a test and puts the old value back afterwards.
class EnvironmentOverride
{
public:
    EnvironmentOverride(const char* name, const QByteArray& value)
        : m_name(name)
        , m_old(qgetenv(name))
        , m_wasSet(qEnvironmentVariableIsSet(name))
    {
        if (value.isNull()) {
            qunsetenv(name);
        } else {
            qputenv(name, value);
        }
    }
    ~EnvironmentOverride()
    {
        if (m_wasSet) {
            qputenv(m_name, m_old);
        } else {
            qunsetenv(m_name);
        }
    }
    EnvironmentOverride(const EnvironmentOverride&) = delete;
    EnvironmentOverride& operator=(const EnvironmentOverride&) = delete;

private:
    const char* m_name;
    QByteArray m_old;
    bool m_wasSet;
};

} // namespace

TEST(SettingsManagerLocation, DefaultFileIsScribeIniInAScribeFolderUnderAppData)
{
    const EnvironmentOverride env("APPDATA", QByteArray("C:\\Users\\Someone\\AppData\\Roaming"));

    EXPECT_EQ(SettingsManager::defaultFilePath(),
              QStringLiteral("C:/Users/Someone/AppData/Roaming/Scribe/scribe.ini"));
}

TEST(SettingsManagerLocation, FallsBackToTheHomeFolderWhenAppDataIsNotSet)
{
    const EnvironmentOverride env("APPDATA", QByteArray());

    const QString path = SettingsManager::defaultFilePath();
    EXPECT_TRUE(path.endsWith(QStringLiteral("/AppData/Roaming/Scribe/scribe.ini")))
        << qPrintable(path);
    EXPECT_TRUE(path.startsWith(QDir::cleanPath(QDir::homePath()))) << qPrintable(path);
}

TEST(SettingsManagerLocation, ComputingTheDefaultPathCreatesNothing)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const EnvironmentOverride env("APPDATA", dir.path().toUtf8());

    const QString path = SettingsManager::defaultFilePath();

    EXPECT_FALSE(QFileInfo::exists(path));
    EXPECT_FALSE(QFileInfo::exists(QFileInfo(path).absolutePath()));
}
