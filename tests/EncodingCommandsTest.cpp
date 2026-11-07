#include "EncodingCommands.h"

#include "DocumentController.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using scribe::core::CommandRegistry;
using scribe::core::Encoding;
using scribe::core::EncodingInfo;
using scribe::core::LineEnding;
using scribe::editor::EditorTabs;

// The Encoding menu's commands and the safety rule behind them. Changing the encoding or line
// ending changes how the NEXT SAVE writes the file, so several tests save and check the bytes.
class EncodingCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void tableCoversEveryEncodingOnceWithUniqueIds();
    void commandsAreRegisteredWithTheirMenuText();
    void exactlyOneEncodingAndOneLineEndingIsCheckedAtATime();
    void checkMarksFollowTheActiveTab();

    void choosingAnEncodingChangesItAndMarksTheDocumentModified();
    void choosingTheCurrentEncodingChangesNothing();
    void choosingALineEndingChangesItAndMarksTheDocumentModified();
    void choosingTheCurrentLineEndingChangesNothing();
    void otherTabsAreNotAffected();

    void ansiIsRefusedWhenTheTextHasCharactersItCannotStore();
    void ansiIsAllowedForPlainText();
    void unicodeEncodingsAcceptEmoji();
    void aRefusedChangeDoesNotBlockALaterValidOne();

    void savingAfterTheChangeWritesTheNewEncodingAndLineEnding();
    void savingAsUtf8WithBomWritesTheBom();
    void savedFileIsDetectedAsTheChosenFormat();
    void unchangedFilesStillSaveByteForByte();

private:
    QString makeFile(const QString& name, const QByteArray& bytes) const;
    QByteArray read(const QString& name) const;
    QString path(const QString& name) const { return m_dir->filePath(name); }

    // Everything a window would have, minus the window.
    struct Fixture
    {
        EditorTabs tabs;
        CommandRegistry registry;
        FileDialogs dialogs;
        QStringList errors;
        std::unique_ptr<DocumentController> controller;

        Fixture()
        {
            dialogs.showError = [this](QWidget*, const QString&, const QString& message) {
                errors.append(message);
            };
            controller = std::make_unique<DocumentController>(tabs, dialogs, nullptr);
            EncodingCommands::registerIn(registry, *controller, tabs);
        }

        [[nodiscard]] scribe::editor::EditorWidget* editor() const { return tabs.currentEditor(); }
        [[nodiscard]] const EncodingInfo& encoding() const
        {
            return tabs.currentEditor()->fileDocument().encoding();
        }
        [[nodiscard]] LineEnding lineEnding() const
        {
            return tabs.currentEditor()->fileDocument().lineEnding();
        }
        bool run(QLatin1String id) const { return registry.run(id); }
    };

    std::unique_ptr<QTemporaryDir> m_dir;
};

namespace
{
constexpr QLatin1String kUtf8("encoding.utf8");
constexpr QLatin1String kUtf8Bom("encoding.utf8Bom");
constexpr QLatin1String kUtf16Le("encoding.utf16Le");
constexpr QLatin1String kUtf16Be("encoding.utf16Be");
constexpr QLatin1String kAnsi("encoding.ansi");
constexpr QLatin1String kCrlf("lineEnding.crlf");
constexpr QLatin1String kLf("lineEnding.lf");
constexpr QLatin1String kCr("lineEnding.cr");

QByteArray hex(const char* digits)
{
    return QByteArray::fromHex(digits);
}
} // namespace

void EncodingCommandsTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

QString EncodingCommandsTest::makeFile(const QString& name, const QByteArray& bytes) const
{
    QFile file(path(name));
    file.open(QIODevice::WriteOnly);
    file.write(bytes);
    return path(name);
}

QByteArray EncodingCommandsTest::read(const QString& name) const
{
    QFile file(path(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void EncodingCommandsTest::tableCoversEveryEncodingOnceWithUniqueIds()
{
    const auto& encodings = EncodingCommands::encodings();
    QCOMPARE(encodings.size(), 7);

    QSet<QString> ids;
    QList<EncodingInfo> infos;
    for (const auto& entry : encodings) {
        ids.insert(entry.id);
        QVERIFY2(!infos.contains(entry.encoding), qPrintable(entry.id));
        infos.append(entry.encoding);
    }
    for (const auto& entry : EncodingCommands::lineEndings()) {
        ids.insert(entry.id);
    }
    QCOMPARE(ids.size(), 10); // 7 encodings + 3 line endings, no id reused

    // Every way the editor can read a file is a way it can write one (ANSI never has a BOM).
    for (const EncodingInfo& wanted :
         {EncodingInfo{Encoding::Utf8, false}, EncodingInfo{Encoding::Utf8, true},
          EncodingInfo{Encoding::Utf16Le, true}, EncodingInfo{Encoding::Utf16Le, false},
          EncodingInfo{Encoding::Utf16Be, true}, EncodingInfo{Encoding::Utf16Be, false},
          EncodingInfo{Encoding::Ansi, false}}) {
        QVERIFY2(infos.contains(wanted), qPrintable(scribe::core::encodingName(wanted)));
    }
}

void EncodingCommandsTest::commandsAreRegisteredWithTheirMenuText()
{
    Fixture f;
    QCOMPARE(f.registry.commands().size(), size_t{10});

    for (const auto& entry : EncodingCommands::encodings()) {
        const auto* command = f.registry.find(entry.id);
        if (command == nullptr) {
            QFAIL(qPrintable(entry.id));
        }
        QCOMPARE(command->title, scribe::core::encodingName(entry.encoding));
        QVERIFY(static_cast<bool>(command->checkedWhen)); // radio-style: shows a check mark
    }
    QCOMPARE(f.registry.find(kCrlf)->title, QStringLiteral("&Windows (CRLF)"));
    QCOMPARE(f.registry.find(kLf)->title, QStringLiteral("&Unix (LF)"));
    QCOMPARE(f.registry.find(kCr)->title, QStringLiteral("&Old Mac (CR)"));
}

void EncodingCommandsTest::exactlyOneEncodingAndOneLineEndingIsCheckedAtATime()
{
    Fixture f;

    const auto checkedEncodings = [&] {
        QStringList checked;
        for (const auto& entry : EncodingCommands::encodings()) {
            if (f.registry.isChecked(entry.id)) {
                checked << entry.id;
            }
        }
        return checked;
    };
    const auto checkedLineEndings = [&] {
        QStringList checked;
        for (const auto& entry : EncodingCommands::lineEndings()) {
            if (f.registry.isChecked(entry.id)) {
                checked << entry.id;
            }
        }
        return checked;
    };

    QCOMPARE(checkedEncodings(), (QStringList{kUtf8}));   // a new document: UTF-8 ...
    QCOMPARE(checkedLineEndings(), (QStringList{kCrlf})); // ... with Windows line endings

    f.run(kUtf16Be);
    f.run(kCr);
    QCOMPARE(checkedEncodings(), (QStringList{kUtf16Be}));
    QCOMPARE(checkedLineEndings(), (QStringList{kCr}));
}

void EncodingCommandsTest::checkMarksFollowTheActiveTab()
{
    Fixture f;
    f.run(kUtf16Le);
    f.tabs.addEditor(); // a new tab starts as UTF-8
    QVERIFY(f.registry.isChecked(kUtf8));
    QVERIFY(!f.registry.isChecked(kUtf16Le));

    f.tabs.setCurrentIndex(0);
    QVERIFY(f.registry.isChecked(kUtf16Le));
    QVERIFY(!f.registry.isChecked(kUtf8));
}

void EncodingCommandsTest::choosingAnEncodingChangesItAndMarksTheDocumentModified()
{
    Fixture f;
    QVERIFY(!f.editor()->isModified());

    QVERIFY(f.run(kUtf8Bom));

    QCOMPARE(f.encoding(), (EncodingInfo{Encoding::Utf8, true}));
    QVERIFY(f.editor()->isModified());
    QCOMPARE(f.tabs.tabText(0), QStringLiteral("Untitled *")); // the user sees unsaved work
}

void EncodingCommandsTest::choosingTheCurrentEncodingChangesNothing()
{
    Fixture f;
    QVERIFY(f.run(kUtf8));

    QVERIFY(!f.editor()->isModified());
    QVERIFY(f.errors.isEmpty());
}

void EncodingCommandsTest::choosingALineEndingChangesItAndMarksTheDocumentModified()
{
    Fixture f;

    QVERIFY(f.run(kLf));

    QCOMPARE(f.lineEnding(), LineEnding::Lf);
    QVERIFY(f.editor()->isModified());
}

void EncodingCommandsTest::choosingTheCurrentLineEndingChangesNothing()
{
    Fixture f;
    QVERIFY(f.run(kCrlf)); // a new document already uses CRLF
    QVERIFY(!f.editor()->isModified());
}

void EncodingCommandsTest::otherTabsAreNotAffected()
{
    Fixture f;
    f.tabs.addEditor();
    f.run(kAnsi);
    f.run(kLf);

    QCOMPARE(f.tabs.editorAt(0)->fileDocument().encoding(), (EncodingInfo{Encoding::Utf8, false}));
    QCOMPARE(f.tabs.editorAt(0)->fileDocument().lineEnding(), LineEnding::Crlf);
    QVERIFY(!f.tabs.editorAt(0)->isModified());
}

void EncodingCommandsTest::ansiIsRefusedWhenTheTextHasCharactersItCannotStore()
{
    Fixture f;
    f.editor()->insertPlainText(QStringLiteral("smile \U0001F600")); // no ANSI code page has this

    QVERIFY(f.run(kAnsi)); // the command ran; the controller declined the change

    QCOMPARE(f.encoding(), (EncodingInfo{Encoding::Utf8, false}));
    QVERIFY(f.registry.isChecked(kUtf8));
    QCOMPARE(f.errors.size(), 1);
    QVERIFY2(f.errors.first().contains("ANSI"), qPrintable(f.errors.first()));
}

void EncodingCommandsTest::ansiIsAllowedForPlainText()
{
    Fixture f;
    f.editor()->insertPlainText(QStringLiteral("plain ascii text\nsecond line"));

    QVERIFY(f.run(kAnsi));

    QCOMPARE(f.encoding(), (EncodingInfo{Encoding::Ansi, false}));
    QVERIFY(f.errors.isEmpty());
}

void EncodingCommandsTest::unicodeEncodingsAcceptEmoji()
{
    Fixture f;
    f.editor()->insertPlainText(QStringLiteral("smile \U0001F600 and €"));

    for (const auto id : {kUtf8Bom, kUtf16Le, kUtf16Be, kUtf8}) {
        QVERIFY2(f.run(id), id.data());
        QVERIFY(f.registry.isChecked(id));
    }
    QVERIFY(f.errors.isEmpty());
}

void EncodingCommandsTest::aRefusedChangeDoesNotBlockALaterValidOne()
{
    Fixture f;
    f.editor()->insertPlainText(QStringLiteral("€")); // the euro sign exists in Windows-1252 only

    f.run(kUtf16Le);
    f.editor()->insertPlainText(QStringLiteral("\U0001F600"));
    f.run(kAnsi);
    QVERIFY(f.registry.isChecked(kUtf16Le)); // refused, so still UTF-16

    f.run(kUtf8Bom);
    QVERIFY(f.registry.isChecked(kUtf8Bom));
}

void EncodingCommandsTest::savingAfterTheChangeWritesTheNewEncodingAndLineEnding()
{
    Fixture f;
    const QString file = makeFile("a.txt", "one\ntwo\n"); // UTF-8, LF
    QVERIFY(f.tabs.openFile(file).ok());
    QCOMPARE(f.lineEnding(), LineEnding::Lf);

    f.run(kUtf16Le);
    f.run(kCrlf);
    QVERIFY(f.editor()->saveFile().ok());

    // FF FE (UTF-16 LE BOM), then each character as two bytes, each line break as CR LF.
    QCOMPARE(read("a.txt"), hex("FFFE") + hex("6F006E006500") + hex("0D000A00") +
                                hex("74007700 6F00") + hex("0D000A00"));
    QVERIFY(!f.editor()->isModified());
}

void EncodingCommandsTest::savingAsUtf8WithBomWritesTheBom()
{
    Fixture f;
    const QString file = makeFile("a.txt", "caf\xC3\xA9\r\nend"); // UTF-8 without BOM, CRLF
    QVERIFY(f.tabs.openFile(file).ok());

    f.run(kUtf8Bom);
    QVERIFY(f.editor()->saveFile().ok());

    QCOMPARE(read("a.txt"), hex("EFBBBF") + "caf\xC3\xA9\r\nend");
}

void EncodingCommandsTest::savedFileIsDetectedAsTheChosenFormat()
{
    Fixture f;
    f.editor()->insertPlainText(QStringLiteral("round\ntrip"));
    f.run(kUtf16Be);
    f.run(kCr);
    QVERIFY(f.editor()->saveFileAs(path("rt.txt")).ok());

    // Open the same bytes again in a fresh set of tabs: they must read back as what was chosen.
    EditorTabs reopened;
    QVERIFY(reopened.openFile(path("rt.txt")).ok());
    QCOMPARE(reopened.currentEditor()->fileDocument().encoding(),
             (EncodingInfo{Encoding::Utf16Be, true}));
    QCOMPARE(reopened.currentEditor()->fileDocument().lineEnding(), LineEnding::Cr);
    QCOMPARE(reopened.currentEditor()->toPlainText(), QStringLiteral("round\ntrip"));
}

void EncodingCommandsTest::unchangedFilesStillSaveByteForByte()
{
    // Choosing a format must not be needed to keep one: a plain open + save leaves bytes alone.
    Fixture f;
    const QByteArray original = hex("FFFE") + hex("680069000D000A00");
    const QString file = makeFile("keep.txt", original);
    QVERIFY(f.tabs.openFile(file).ok());

    QVERIFY(f.editor()->saveFile().ok());

    QCOMPARE(read("keep.txt"), original);
}

QTEST_MAIN(EncodingCommandsTest)

#include "EncodingCommandsTest.moc"
