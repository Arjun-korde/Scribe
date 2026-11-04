#include "Document.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

using scribe::core::Document;
using scribe::core::DocumentError;
using scribe::core::Encoding;
using scribe::core::EncodingInfo;
using scribe::core::LineEnding;

namespace
{

QByteArray hex(const char* digits)
{
    return QByteArray::fromHex(digits);
}

class DocumentTest : public testing::Test
{
protected:
    void SetUp() override { ASSERT_TRUE(m_dir.isValid()); }

    QString pathFor(const QString& name) const { return m_dir.filePath(name); }

    QString writeFile(const QString& name, const QByteArray& bytes) const
    {
        const QString path = pathFor(name);
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(bytes);
        return path;
    }

    static QByteArray readFile(const QString& path)
    {
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::ReadOnly));
        return file.readAll();
    }

private:
    QTemporaryDir m_dir;
};

} // namespace

TEST_F(DocumentTest, NewDocumentHasWindowsDefaults)
{
    const Document doc;
    EXPECT_EQ(doc.encoding(), (EncodingInfo{Encoding::Utf8, false}));
    EXPECT_EQ(doc.lineEnding(), LineEnding::Crlf);
    EXPECT_FALSE(doc.isModified());
    EXPECT_TRUE(doc.filePath().isEmpty());
    EXPECT_EQ(doc.backend().length(), 0);
}

TEST_F(DocumentTest, LoadNormalizesTextAndRemembersFormat)
{
    const QString path = writeFile("a.txt", hex("EFBBBF") + "caf\xC3\xA9\r\nline2");
    Document doc;
    ASSERT_TRUE(doc.load(path).ok());

    EXPECT_EQ(doc.backend().text(), QStringLiteral("café\nline2"));
    EXPECT_EQ(doc.encoding(), (EncodingInfo{Encoding::Utf8, true}));
    EXPECT_EQ(doc.lineEnding(), LineEnding::Crlf);
    EXPECT_EQ(doc.filePath(), path);
    EXPECT_FALSE(doc.isModified());
}

TEST_F(DocumentTest, LoadOfMissingFileFailsAndKeepsDocument)
{
    Document doc;
    doc.setText(QStringLiteral("keep me"));

    const auto result = doc.load(pathFor("missing.txt"));
    EXPECT_EQ(result.error, DocumentError::CannotRead);
    EXPECT_EQ(doc.backend().text(), QStringLiteral("keep me"));
    EXPECT_TRUE(doc.isModified());
}

TEST_F(DocumentTest, LoadRefusesBytesThatWouldChangeOnSave)
{
    // UTF-16 LE with a dangling odd byte: decoding would drop it, saving would lose it.
    const QString path = writeFile("odd.txt", hex("FFFE6800690000"));
    Document doc;
    EXPECT_EQ(doc.load(path).error, DocumentError::InvalidEncoding);
    EXPECT_TRUE(doc.filePath().isEmpty());
}

TEST_F(DocumentTest, UnchangedFilesSaveByteForByte)
{
    const QByteArray files[] = {
        "plain\r\ntext\r\n",         "unix\nlines\n", "old mac\rlines\r", "no line break", "",
        hex("EFBBBF") + "bom\r\n",
        hex("636166E90D0A"),         // ANSI "caf\xE9" CRLF
        hex("FFFE680069000D000A00"), // UTF-16 LE BOM
        hex("FEFF00680069000D000A"), // UTF-16 BE BOM
        hex("680069000D000A00"),     // UTF-16 LE, no BOM
        hex("00680069000D000A"),     // UTF-16 BE, no BOM
        hex("E282ACF09F98800A"),     // UTF-8 with euro and emoji
    };

    int index = 0;
    for (const QByteArray& original : files) {
        const QString path = writeFile(QStringLiteral("rt%1.txt").arg(index++), original);
        Document doc;
        ASSERT_TRUE(doc.load(path).ok()) << qPrintable(original.toHex());
        ASSERT_TRUE(doc.save().ok());
        EXPECT_EQ(readFile(path), original) << qPrintable(original.toHex());
    }
}

TEST_F(DocumentTest, MixedLineEndingsAreSavedAsTheDominantOne)
{
    const QString path = writeFile("mixed.txt", "a\nb\nc\r\nd");
    Document doc;
    ASSERT_TRUE(doc.load(path).ok());
    EXPECT_EQ(doc.lineEnding(), LineEnding::Lf);

    ASSERT_TRUE(doc.save().ok());
    EXPECT_EQ(readFile(path), QByteArray("a\nb\nc\nd"));
}

TEST_F(DocumentTest, EditsMarkModifiedAndSaveClearsIt)
{
    const QString path = writeFile("e.txt", "abc");
    Document doc;
    ASSERT_TRUE(doc.load(path).ok());

    EXPECT_FALSE(doc.insert(10, QStringLiteral("x")));
    EXPECT_FALSE(doc.isModified());

    EXPECT_TRUE(doc.insert(3, QStringLiteral("d")));
    EXPECT_TRUE(doc.isModified());
    EXPECT_TRUE(doc.save().ok());
    EXPECT_FALSE(doc.isModified());

    EXPECT_TRUE(doc.remove(0, 1));
    EXPECT_TRUE(doc.isModified());
    ASSERT_TRUE(doc.save().ok());
    EXPECT_EQ(readFile(path), QByteArray("bcd"));
}

TEST_F(DocumentTest, InsertedTextIsNormalizedToLf)
{
    Document doc;
    doc.setText(QStringLiteral("a\r\nb"));
    EXPECT_TRUE(doc.insert(1, QStringLiteral("\r")));
    EXPECT_EQ(doc.backend().text(), QStringLiteral("a\n\nb"));
}

TEST_F(DocumentTest, ConvertingLineEndingsChangesTheSavedFile)
{
    const QString path = writeFile("c.txt", "a\nb\n");
    Document doc;
    ASSERT_TRUE(doc.load(path).ok());

    doc.setLineEnding(LineEnding::Crlf);
    EXPECT_TRUE(doc.isModified());
    ASSERT_TRUE(doc.save().ok());
    EXPECT_EQ(readFile(path), QByteArray("a\r\nb\r\n"));
}

TEST_F(DocumentTest, SavingWithADifferentEncodingChangesTheSavedFile)
{
    const QString path = writeFile("u.txt", "hi\n");
    Document doc;
    ASSERT_TRUE(doc.load(path).ok());

    doc.setEncoding({Encoding::Utf16Le, true});
    ASSERT_TRUE(doc.save().ok());
    EXPECT_EQ(readFile(path), hex("FFFE68006900") + hex("0A00"));
}

TEST_F(DocumentTest, SettingTheSameFormatDoesNotMarkModified)
{
    Document doc;
    doc.setEncoding({Encoding::Utf8, false});
    doc.setLineEnding(LineEnding::Crlf);
    EXPECT_FALSE(doc.isModified());
}

TEST_F(DocumentTest, UnencodableTextFailsAndLeavesTheFileAlone)
{
    const QString path = writeFile("keep.txt", "original");
    Document doc;
    ASSERT_TRUE(doc.load(path).ok());
    doc.setEncoding({Encoding::Ansi, false});
    doc.setText(QStringLiteral("smile \U0001F600"));

    EXPECT_EQ(doc.save().error, DocumentError::CannotEncode);
    EXPECT_EQ(readFile(path), QByteArray("original"));
    EXPECT_TRUE(doc.isModified());
}

TEST_F(DocumentTest, SaveAsWritesNewFileAndAdoptsItsPath)
{
    Document doc;
    doc.setText(QStringLiteral("hello"));

    const QString path = pathFor("new.txt");
    ASSERT_TRUE(doc.saveAs(path).ok());
    EXPECT_EQ(doc.filePath(), path);
    EXPECT_FALSE(doc.isModified());
    EXPECT_EQ(readFile(path), QByteArray("hello"));
}

TEST_F(DocumentTest, FailedSaveAsKeepsOldPathAndModifiedFlag)
{
    const QString original = writeFile("orig.txt", "x");
    Document doc;
    ASSERT_TRUE(doc.load(original).ok());
    doc.setText(QStringLiteral("changed"));

    const auto result = doc.saveAs(pathFor("no-such-folder/out.txt"));
    EXPECT_EQ(result.error, DocumentError::CannotWrite);
    EXPECT_EQ(doc.filePath(), original);
    EXPECT_TRUE(doc.isModified());
    EXPECT_EQ(readFile(original), QByteArray("x"));
}

TEST_F(DocumentTest, SaveWithoutAPathFails)
{
    Document doc;
    doc.setText(QStringLiteral("x"));
    EXPECT_EQ(doc.save().error, DocumentError::CannotWrite);
    EXPECT_TRUE(doc.isModified());
}
