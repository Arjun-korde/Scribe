#include "RecentFiles.h"

#include "SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

using scribe::core::RecentFiles;
using scribe::core::SettingsManager;

namespace
{

// Absolute paths that need not exist: the list never touches the files themselves.
QString file(int number)
{
    return QStringLiteral("C:/docs/file%1.txt").arg(number);
}

} // namespace

TEST(RecentFiles, StartsEmpty)
{
    const RecentFiles recent;
    EXPECT_TRUE(recent.isEmpty());
    EXPECT_EQ(recent.size(), 0);
    EXPECT_EQ(recent.maxItems(), 10);
}

TEST(RecentFiles, NewestComesFirst)
{
    RecentFiles recent;
    EXPECT_TRUE(recent.add(file(1)));
    EXPECT_TRUE(recent.add(file(2)));
    EXPECT_TRUE(recent.add(file(3)));
    EXPECT_EQ(recent.paths(), (QStringList{file(3), file(2), file(1)}));
}

TEST(RecentFiles, UsingAFileAgainMovesItToTheTopWithoutDuplicating)
{
    RecentFiles recent;
    recent.add(file(1));
    recent.add(file(2));
    recent.add(file(3));
    EXPECT_TRUE(recent.add(file(1)));
    EXPECT_EQ(recent.paths(), (QStringList{file(1), file(3), file(2)}));
}

TEST(RecentFiles, AddingTheFileAlreadyOnTopChangesNothing)
{
    RecentFiles recent;
    recent.add(file(1));
    EXPECT_FALSE(recent.add(file(1)));
    EXPECT_EQ(recent.size(), 1);
}

TEST(RecentFiles, TheOldestEntriesFallOffTheEnd)
{
    RecentFiles recent;
    for (int i = 1; i <= 15; ++i) {
        recent.add(file(i));
    }
    ASSERT_EQ(recent.size(), 10);
    EXPECT_EQ(recent.paths().first(), file(15));
    EXPECT_EQ(recent.paths().last(), file(6));
    EXPECT_FALSE(recent.contains(file(5)));
}

TEST(RecentFiles, TheLimitIsConfigurable)
{
    RecentFiles recent(3);
    for (int i = 1; i <= 5; ++i) {
        recent.add(file(i));
    }
    EXPECT_EQ(recent.paths(), (QStringList{file(5), file(4), file(3)}));
    EXPECT_EQ(RecentFiles(0).maxItems(), 1); // never less than one
    EXPECT_EQ(RecentFiles(-4).maxItems(), 1);
}

TEST(RecentFiles, EmptyPathsAreIgnored)
{
    RecentFiles recent;
    EXPECT_FALSE(recent.add(QString()));
    EXPECT_FALSE(recent.add(QStringLiteral("")));
    EXPECT_FALSE(recent.add(QStringLiteral("   ")));
    EXPECT_TRUE(recent.isEmpty());
}

TEST(RecentFiles, DifferentSpellingsOfOneFileAreOneEntry)
{
    RecentFiles recent;
    recent.add(QStringLiteral("C:\\docs\\notes.txt"));
    recent.add(QStringLiteral("C:/docs/sub/../notes.txt"));
    recent.add(QStringLiteral("C:/docs/./notes.txt"));
    EXPECT_EQ(recent.paths(), (QStringList{QStringLiteral("C:/docs/notes.txt")}));
}

TEST(RecentFiles, ARelativePathIsStoredAsAnAbsoluteOne)
{
    RecentFiles recent;
    recent.add(QStringLiteral("notes.txt"));
    ASSERT_EQ(recent.size(), 1);
    EXPECT_TRUE(QDir::isAbsolutePath(recent.paths().first()));
    EXPECT_TRUE(recent.paths().first().endsWith(QStringLiteral("/notes.txt")));
}

#ifdef Q_OS_WIN
TEST(RecentFiles, LetterCaseDoesNotMatterOnWindows)
{
    RecentFiles recent;
    recent.add(QStringLiteral("C:/Docs/Notes.txt"));
    recent.add(QStringLiteral("c:/docs/other.txt"));
    EXPECT_TRUE(recent.contains(QStringLiteral("C:/DOCS/NOTES.TXT")));
    recent.add(QStringLiteral("C:/DOCS/notes.txt"));
    // One entry, now spelled the way it was used last.
    EXPECT_EQ(
        recent.paths(),
        (QStringList{QStringLiteral("C:/DOCS/notes.txt"),
                     QStringLiteral("C:/docs/other.txt")})); // the drive letter is upper-cased
}
#endif

TEST(RecentFiles, ContainsUnderstandsOtherSpellings)
{
    RecentFiles recent;
    recent.add(file(1));
    EXPECT_TRUE(recent.contains(QStringLiteral("C:\\docs\\file1.txt")));
    EXPECT_FALSE(recent.contains(file(2)));
    EXPECT_FALSE(recent.contains(QString()));
}

TEST(RecentFiles, RemoveTakesOutOneEntry)
{
    RecentFiles recent;
    recent.add(file(1));
    recent.add(file(2));
    recent.add(file(3));
    EXPECT_TRUE(recent.remove(QStringLiteral("C:\\docs\\file2.txt")));
    EXPECT_EQ(recent.paths(), (QStringList{file(3), file(1)}));
    EXPECT_FALSE(recent.remove(file(2))); // already gone
    EXPECT_FALSE(recent.remove(QString()));
}

TEST(RecentFiles, ClearEmptiesTheList)
{
    RecentFiles recent;
    EXPECT_FALSE(recent.clear());
    recent.add(file(1));
    EXPECT_TRUE(recent.clear());
    EXPECT_TRUE(recent.isEmpty());
}

TEST(RecentFiles, AFreedSlotIsUsableAfterRemove)
{
    RecentFiles recent(2);
    recent.add(file(1));
    recent.add(file(2));
    recent.remove(file(2));
    recent.add(file(3));
    EXPECT_EQ(recent.paths(), (QStringList{file(3), file(1)}));
}

// ---- Saving and loading ---------------------------------------------------------------------

class RecentFilesStoreTest : public testing::Test
{
protected:
    void SetUp() override { ASSERT_TRUE(m_dir.isValid()); }

    // A new file name each time: Qt caches parsed INI files (see the SettingsManager tests).
    QString newPath() { return m_dir.filePath(QStringLiteral("r%1.ini").arg(++m_counter)); }

    QString writeIni(const QByteArray& bytes)
    {
        const QString path = newPath();
        QFile out(path);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(bytes);
        }
        return path;
    }

    QTemporaryDir m_dir;
    int m_counter = 0;
};

TEST_F(RecentFilesStoreTest, NothingSavedGivesAnEmptyList)
{
    const SettingsManager settings(newPath());
    EXPECT_TRUE(RecentFiles::load(settings).isEmpty());
}

TEST_F(RecentFilesStoreTest, LoadingNeverCreatesTheFile)
{
    const QString path = newPath();
    {
        const SettingsManager settings(path);
        (void)RecentFiles::load(settings);
    }
    EXPECT_FALSE(QFile::exists(path));
}

TEST_F(RecentFilesStoreTest, ASavedListComesBackInTheSameOrder)
{
    const QString path = newPath();
    RecentFiles recent;
    recent.add(file(1));
    recent.add(file(2));
    recent.add(file(3));
    {
        SettingsManager settings(path);
        recent.save(settings);
    }
    const SettingsManager settings(path);
    EXPECT_EQ(RecentFiles::load(settings).paths(), recent.paths());
}

TEST_F(RecentFilesStoreTest, PathsWithCommasQuotesAndSpacesSurvive)
{
    const QString path = newPath();
    RecentFiles recent;
    recent.add(QStringLiteral("C:/My Docs/a, b; \"c\" [d] #e=f.txt"));
    recent.add(QStringLiteral("D:/\u00fcn\u00efc\u00f6d\u00e9/\u65e5\u672c\u8a9e.txt"));
    {
        SettingsManager settings(path);
        recent.save(settings);
    }
    const SettingsManager settings(path);
    EXPECT_EQ(RecentFiles::load(settings).paths(), recent.paths());
}

TEST_F(RecentFilesStoreTest, AnEmptyListRemovesTheSavedOne)
{
    SettingsManager settings(newPath());
    RecentFiles recent;
    recent.add(file(1));
    recent.save(settings);
    EXPECT_FALSE(RecentFiles::load(settings).isEmpty());
    recent.clear();
    recent.save(settings);
    EXPECT_TRUE(RecentFiles::load(settings).isEmpty());
}

TEST_F(RecentFilesStoreTest, TheFileIsReadable)
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        RecentFiles recent;
        recent.add(file(1));
        recent.add(file(2));
        recent.save(settings);
    }
    QFile in(path);
    ASSERT_TRUE(in.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(in.readAll());
    EXPECT_TRUE(text.contains(QStringLiteral("[recent]"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("C:/docs/file2.txt"))) << qPrintable(text);
}

TEST_F(RecentFilesStoreTest, DamagedEntriesAreSkippedAndTheRestKept)
{
    // Entry 2 is empty, 3 is relative, 5 repeats entry 1 with a different spelling.
    const SettingsManager settings(writeIni("[recent]\nfiles\\size=5\n"
                                            "files\\1\\value=C:/docs/a.txt\n"
                                            "files\\2\\value=\n"
                                            "files\\3\\value=relative/b.txt\n"
                                            "files\\4\\value=C:/docs/c.txt\n"
                                            "files\\5\\value=C:/docs/./a.txt\n"));
    const RecentFiles recent = RecentFiles::load(settings);
    EXPECT_EQ(recent.paths(),
              (QStringList{QStringLiteral("C:/docs/a.txt"), QStringLiteral("C:/docs/c.txt")}));
}

TEST_F(RecentFilesStoreTest, ALongSavedListKeepsTheNewestEntries)
{
    QByteArray text = "[recent]\nfiles\\size=14\n";
    for (int i = 1; i <= 14; ++i) {
        text += "files\\" + QByteArray::number(i) + "\\value=C:/docs/file" + QByteArray::number(i) +
                ".txt\n";
    }
    const SettingsManager settings(writeIni(text));
    const RecentFiles recent = RecentFiles::load(settings);
    ASSERT_EQ(recent.size(), 10);
    EXPECT_EQ(recent.paths().first(), file(1)); // first in the file is the newest
    EXPECT_EQ(recent.paths().last(), file(10));
}

TEST_F(RecentFilesStoreTest, LoadingHonoursASmallerLimit)
{
    const QString path = newPath();
    RecentFiles recent;
    for (int i = 1; i <= 5; ++i) {
        recent.add(file(i));
    }
    {
        SettingsManager settings(path);
        recent.save(settings);
    }
    const SettingsManager settings(path);
    EXPECT_EQ(RecentFiles::load(settings, 2).paths(), (QStringList{file(5), file(4)}));
}
