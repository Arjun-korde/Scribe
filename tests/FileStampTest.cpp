#include "FileStamp.h"

#include "Document.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

using scribe::core::Document;
using scribe::core::FileStamp;

namespace
{

class FileStampTest : public testing::Test
{
protected:
    void SetUp() override { ASSERT_TRUE(m_dir.isValid()); }

    QString dirPath() const { return m_dir.path(); }
    QString pathFor(const QString& name) const { return m_dir.filePath(name); }

    QString writeFile(const QString& name, const QByteArray& bytes) const
    {
        const QString path = pathFor(name);
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(bytes);
        return path;
    }

private:
    QTemporaryDir m_dir;
};

} // namespace

TEST_F(FileStampTest, missingFileAndEmptyPathDoNotExist)
{
    EXPECT_FALSE(FileStamp::of(pathFor("nope.txt")).exists);
    EXPECT_FALSE(FileStamp::of(QString()).exists);
    EXPECT_EQ(FileStamp::of(pathFor("nope.txt")), FileStamp{});
}

TEST_F(FileStampTest, aFolderIsNotAFile)
{
    EXPECT_FALSE(FileStamp::of(dirPath()).exists);
}

TEST_F(FileStampTest, recordsSizeAndExistence)
{
    const QString path = writeFile("a.txt", "hello");
    const FileStamp stamp = FileStamp::of(path);
    EXPECT_TRUE(stamp.exists);
    EXPECT_EQ(stamp.size, 5);
    EXPECT_GT(stamp.modifiedMs, 0);
}

TEST_F(FileStampTest, unchangedFileGivesEqualStamps)
{
    const QString path = writeFile("a.txt", "hello");
    EXPECT_EQ(FileStamp::of(path), FileStamp::of(path));
}

TEST_F(FileStampTest, differentSizeGivesDifferentStamps)
{
    const QString path = writeFile("a.txt", "hello");
    const FileStamp before = FileStamp::of(path);
    writeFile("a.txt", "hello world");
    EXPECT_NE(FileStamp::of(path), before);
}

TEST_F(FileStampTest, sameSizeButNewerModificationTimeGivesDifferentStamps)
{
    const QString path = writeFile("a.txt", "hello");
    const FileStamp before = FileStamp::of(path);

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadWrite));
    ASSERT_TRUE(file.setFileTime(QDateTime::currentDateTimeUtc().addSecs(60),
                                 QFileDevice::FileModificationTime));
    file.close();

    const FileStamp after = FileStamp::of(path);
    EXPECT_EQ(after.size, before.size);
    EXPECT_NE(after, before);
}

TEST_F(FileStampTest, deletedFileDiffersFromTheStampItHad)
{
    const QString path = writeFile("a.txt", "hello");
    const FileStamp before = FileStamp::of(path);
    ASSERT_TRUE(QFile::remove(path));
    EXPECT_NE(FileStamp::of(path), before);
    EXPECT_FALSE(FileStamp::of(path).exists);
}

TEST_F(FileStampTest, loadRecordsTheStamp)
{
    const QString path = writeFile("a.txt", "hello");
    Document document;
    ASSERT_TRUE(document.load(path).ok());
    EXPECT_EQ(document.diskStamp(), FileStamp::of(path));
}

TEST_F(FileStampTest, failedLoadKeepsTheOldStamp)
{
    const QString path = writeFile("a.txt", "hello");
    Document document;
    ASSERT_TRUE(document.load(path).ok());
    const FileStamp before = document.diskStamp();

    EXPECT_FALSE(document.load(pathFor("missing.txt")).ok());
    EXPECT_EQ(document.diskStamp(), before);
}

TEST_F(FileStampTest, saveRecordsTheStampOfOurOwnWrite)
{
    const QString path = writeFile("a.txt", "hello");
    Document document;
    ASSERT_TRUE(document.load(path).ok());
    document.setText("a much longer text than before");
    const FileStamp beforeSave = document.diskStamp();

    ASSERT_TRUE(document.save().ok());
    EXPECT_NE(document.diskStamp(), beforeSave); // the file really changed (longer text)
    EXPECT_EQ(document.diskStamp(), FileStamp::of(path));
}

TEST_F(FileStampTest, saveAsRecordsTheStampOfTheNewFile)
{
    Document document;
    document.setText("text");
    const QString path = pathFor("new.txt");
    ASSERT_TRUE(document.saveAs(path).ok());
    EXPECT_EQ(document.diskStamp(), FileStamp::of(path));
}

TEST_F(FileStampTest, anExternalWriteIsVisibleAsADifferentStamp)
{
    const QString path = writeFile("a.txt", "hello");
    Document document;
    ASSERT_TRUE(document.load(path).ok());

    writeFile("a.txt", "changed by someone else");
    EXPECT_NE(FileStamp::of(path), document.diskStamp());
}

TEST_F(FileStampTest, refreshDiskStampAcceptsTheCurrentState)
{
    const QString path = writeFile("a.txt", "hello");
    Document document;
    ASSERT_TRUE(document.load(path).ok());
    writeFile("a.txt", "changed by someone else");

    document.refreshDiskStamp();
    EXPECT_EQ(document.diskStamp(), FileStamp::of(path));
}
