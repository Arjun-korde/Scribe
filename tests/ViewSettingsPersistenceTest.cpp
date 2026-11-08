#include "ViewSettingsPersistence.h"

#include "EditorTabs.h"
#include "EditorWidget.h"
#include "SettingsManager.h"
#include "ViewSettingsStore.h"

#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QtTest>

#include <chrono>
#include <memory>

using scribe::core::SettingsManager;
using scribe::editor::EditorTabs;
using scribe::editor::ViewSettings;
using scribe::editor::ViewSettingsPersistence;
using scribe::editor::ViewSettingsStore;

using namespace std::chrono_literals;

class ViewSettingsPersistenceTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    // ViewSettingsStore: reading and writing the file.
    void loadsTheDefaultsWhenNothingIsSaved();
    void loadingNeverCreatesTheFile();
    void whatWasSavedIsLoadedAgain();
    void aFractionalFontSizeRoundTrips();
    void boldAndItalicAreRemembered();
    void theFileIsReadableAndUsesTheViewSection();
    void aHandEditedFileIsLoaded();
    void invalidValuesGiveDefaultsButDoNotSpoilTheOthers();
    void zoomOutsideTheRangeGivesTheDefault();
    void fontSizeLimitsAreChecked();
    void fontWeightLimitsAreChecked();
    void aFontThatIsNotInstalledFallsBackToTheDefaultFamily();
    void anEmptyFamilyFallsBackToTheDefaultFamily();
    void aPixelSizedFontSavesNoSizeAndDropsAnOldOne();
    void savingToAnUnwritableLocationDoesNotCrash();

    // ViewSettingsPersistence: loading at start and saving after changes.
    void savedSettingsAreAppliedToTheTabs();
    void settingsAreAppliedToTabsOpenedLater();
    void startingWithoutASavedFileLeavesTheDefaultsAndWritesNothing();
    void loadingDoesNotRewriteTheFile();
    void aChangeIsSavedAfterAShortWait();
    void aChangeIsNotWrittenImmediately();
    void aBurstOfChangesIsSavedOnceWithTheFinalValue();
    void flushWritesAtOnce();
    void destroyingItWritesWhatIsStillWaiting();
    void nothingIsPendingAfterASave();
    void everyKindOfChangeIsSaved();
    void theSavedFileLoadsBackToTheSameSettings();
    void changesBeforeTheTabsAreDestroyedAreStillSaved();
    void anUnwritableLocationDoesNotCrash();

private:
    // The current settings file. Every writeIni() starts a NEW file name: Qt keeps parsed INI files
    // in a cache and only re-reads one when its size or timestamp changes, so rewriting the same
    // file twice in a split second (as these tests do) could return the stale first version.
    QString iniPath() const
    {
        return m_dir->filePath(QStringLiteral("scribe-%1.ini").arg(m_generation));
    }
    QString iniText() const
    {
        QFile file(iniPath());
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }
    void writeIni(const QByteArray& bytes)
    {
        ++m_generation;
        QFile file(iniPath());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(bytes);
    }
    // Some installed family, preferring an ordinary one: the alphabetically first font on a
    // Windows machine can be an old bitmap font that the text engine cannot even render.
    static QString installedFamily()
    {
        const QStringList installed = QFontDatabase::families();
        for (const QString& preferred :
             {QStringLiteral("Arial"), QStringLiteral("Segoe UI"), QStringLiteral("Consolas")}) {
            if (installed.contains(preferred)) {
                return preferred;
            }
        }
        return installed.first();
    }

    std::unique_ptr<QTemporaryDir> m_dir;
    int m_generation = 0;
};

void ViewSettingsPersistenceTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_generation = 0;
    // Two tests make the file unwritable on purpose; keep the expected warning out of the output.
    QLoggingCategory::setFilterRules(QStringLiteral("scribe.settings.warning=false"));
}

// ---- ViewSettingsStore ----------------------------------------------------------------------

void ViewSettingsPersistenceTest::loadsTheDefaultsWhenNothingIsSaved()
{
    const SettingsManager settings(iniPath());
    QCOMPARE(ViewSettingsStore::load(settings), ViewSettings());
}

void ViewSettingsPersistenceTest::loadingNeverCreatesTheFile()
{
    const SettingsManager settings(iniPath());
    (void)ViewSettingsStore::load(settings);
    QVERIFY(!QFileInfo::exists(iniPath()));
}

void ViewSettingsPersistenceTest::whatWasSavedIsLoadedAgain()
{
    ViewSettings view;
    view.wordWrap = false;
    view.zoomPercent = 170;
    view.font = QFont(installedFamily());
    view.font.setPointSizeF(13.0);
    {
        SettingsManager settings(iniPath());
        ViewSettingsStore::save(settings, view);
    }

    const SettingsManager settings(iniPath());
    const ViewSettings loaded = ViewSettingsStore::load(settings);
    QCOMPARE(loaded.wordWrap, false);
    QCOMPARE(loaded.zoomPercent, 170);
    QCOMPARE(loaded.font.family(), view.font.family());
    QCOMPARE(loaded.font.pointSizeF(), 13.0);
}

void ViewSettingsPersistenceTest::aFractionalFontSizeRoundTrips()
{
    ViewSettings view;
    view.font.setPointSizeF(10.5);
    {
        SettingsManager settings(iniPath());
        ViewSettingsStore::save(settings, view);
    }
    const SettingsManager settings(iniPath());
    QCOMPARE(ViewSettingsStore::load(settings).font.pointSizeF(), 10.5);
}

void ViewSettingsPersistenceTest::boldAndItalicAreRemembered()
{
    ViewSettings view;
    view.font.setBold(true);
    view.font.setItalic(true);
    {
        SettingsManager settings(iniPath());
        ViewSettingsStore::save(settings, view);
    }
    const SettingsManager settings(iniPath());
    const ViewSettings loaded = ViewSettingsStore::load(settings);
    QVERIFY(loaded.font.italic());
    QCOMPARE(static_cast<int>(loaded.font.weight()), static_cast<int>(QFont::Bold));
}

void ViewSettingsPersistenceTest::theFileIsReadableAndUsesTheViewSection()
{
    ViewSettings view;
    view.wordWrap = false;
    view.zoomPercent = 140;
    view.font = QFont(installedFamily());
    view.font.setPointSizeF(12.0);
    {
        SettingsManager settings(iniPath());
        ViewSettingsStore::save(settings, view);
    }

    const QString text = iniText();
    QVERIFY2(text.contains(QStringLiteral("[view]")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("wordWrap=false")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("zoom=140")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("fontSize=12")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("fontFamily=")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("fontWeight=400")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("fontItalic=false")), qPrintable(text));
}

void ViewSettingsPersistenceTest::aHandEditedFileIsLoaded()
{
    writeIni("[view]\nwordWrap=false\nzoom=130\nfontSize=15\nfontItalic=true\n");

    const SettingsManager settings(iniPath());
    const ViewSettings loaded = ViewSettingsStore::load(settings);
    QCOMPARE(loaded.wordWrap, false);
    QCOMPARE(loaded.zoomPercent, 130);
    QCOMPARE(loaded.font.pointSizeF(), 15.0);
    QVERIFY(loaded.font.italic());
}

void ViewSettingsPersistenceTest::invalidValuesGiveDefaultsButDoNotSpoilTheOthers()
{
    writeIni("[view]\nwordWrap=maybe\nzoom=banana\nfontSize=abc\nfontWeight=heavy\n"
             "fontItalic=perhaps\n");

    const SettingsManager settings(iniPath());
    QCOMPARE(ViewSettingsStore::load(settings), ViewSettings());

    // One bad value costs only that value.
    writeIni("[view]\nwordWrap=false\nzoom=banana\nfontSize=14\n");
    const SettingsManager mixed(iniPath());
    const ViewSettings loaded = ViewSettingsStore::load(mixed);
    QCOMPARE(loaded.wordWrap, false);
    QCOMPARE(loaded.zoomPercent, 100);
    QCOMPARE(loaded.font.pointSizeF(), 14.0);
}

void ViewSettingsPersistenceTest::zoomOutsideTheRangeGivesTheDefault()
{
    for (const char* bad : {"5", "9", "501", "600", "-1", "0", "99999999999"}) {
        writeIni(QByteArray("[view]\nzoom=") + bad + "\n");
        const SettingsManager settings(iniPath());
        QCOMPARE(ViewSettingsStore::load(settings).zoomPercent, 100);
    }
    for (const char* good : {"10", "500", "250"}) {
        writeIni(QByteArray("[view]\nzoom=") + good + "\n");
        const SettingsManager settings(iniPath());
        QCOMPARE(ViewSettingsStore::load(settings).zoomPercent, QByteArray(good).toInt());
    }
}

void ViewSettingsPersistenceTest::fontSizeLimitsAreChecked()
{
    const qreal defaultSize = ViewSettings().font.pointSizeF();
    for (const char* bad : {"0", "0.5", "-3", "201", "1000", "nan", "inf", "-inf", "", "1,5"}) {
        writeIni(QByteArray("[view]\nfontSize=") + bad + "\n");
        const SettingsManager settings(iniPath());
        QCOMPARE(ViewSettingsStore::load(settings).font.pointSizeF(), defaultSize);
    }
    for (const char* good : {"1", "200", "8.25"}) {
        writeIni(QByteArray("[view]\nfontSize=") + good + "\n");
        const SettingsManager settings(iniPath());
        QCOMPARE(ViewSettingsStore::load(settings).font.pointSizeF(), QByteArray(good).toDouble());
    }
}

void ViewSettingsPersistenceTest::fontWeightLimitsAreChecked()
{
    const int defaultWeight = static_cast<int>(ViewSettings().font.weight());
    for (const char* bad : {"0", "-5", "1001", "heavy"}) {
        writeIni(QByteArray("[view]\nfontWeight=") + bad + "\n");
        const SettingsManager settings(iniPath());
        QCOMPARE(static_cast<int>(ViewSettingsStore::load(settings).font.weight()), defaultWeight);
    }
    writeIni("[view]\nfontWeight=700\n");
    const SettingsManager settings(iniPath());
    QCOMPARE(static_cast<int>(ViewSettingsStore::load(settings).font.weight()), 700);
}

void ViewSettingsPersistenceTest::aFontThatIsNotInstalledFallsBackToTheDefaultFamily()
{
    writeIni("[view]\nfontFamily=No Such Font 12345\nfontSize=16\nzoom=150\n");

    const SettingsManager settings(iniPath());
    const ViewSettings loaded = ViewSettingsStore::load(settings);

    QCOMPARE(loaded.font.family(), ViewSettings().font.family());
    QCOMPARE(loaded.font.pointSizeF(), 16.0); // the rest of the saved settings still apply
    QCOMPARE(loaded.zoomPercent, 150);
}

void ViewSettingsPersistenceTest::anEmptyFamilyFallsBackToTheDefaultFamily()
{
    writeIni("[view]\nfontFamily=\n");
    const SettingsManager settings(iniPath());
    QCOMPARE(ViewSettingsStore::load(settings).font.family(), ViewSettings().font.family());
}

void ViewSettingsPersistenceTest::aPixelSizedFontSavesNoSizeAndDropsAnOldOne()
{
    ViewSettings sized;
    sized.font.setPointSizeF(12.0);
    SettingsManager settings(iniPath());
    ViewSettingsStore::save(settings, sized);
    QVERIFY(iniText().contains(QStringLiteral("fontSize=12")));

    ViewSettings pixels;
    pixels.font.setPixelSize(20); // no point size any more
    QVERIFY(pixels.font.pointSizeF() <= 0);
    ViewSettingsStore::save(settings, pixels);

    QVERIFY2(!iniText().contains(QStringLiteral("fontSize")), qPrintable(iniText()));
}

void ViewSettingsPersistenceTest::savingToAnUnwritableLocationDoesNotCrash()
{
    QFile blocker(m_dir->filePath(QStringLiteral("blocker")));
    QVERIFY(blocker.open(QIODevice::WriteOnly)); // a file where a folder would be needed
    blocker.close();

    SettingsManager settings(m_dir->filePath(QStringLiteral("blocker/sub/scribe.ini")));
    ViewSettingsStore::save(settings, ViewSettings());

    QVERIFY(!settings.isHealthy());
}

// ---- ViewSettingsPersistence ----------------------------------------------------------------

void ViewSettingsPersistenceTest::savedSettingsAreAppliedToTheTabs()
{
    writeIni("[view]\nwordWrap=false\nzoom=140\nfontSize=14\n");
    SettingsManager settings(iniPath());
    EditorTabs tabs;

    const ViewSettingsPersistence persistence(tabs, settings, 30ms);

    QCOMPARE(tabs.viewSettings().wordWrap, false);
    QCOMPARE(tabs.viewSettings().zoomPercent, 140);
    QCOMPARE(tabs.viewSettings().font.pointSizeF(), 14.0);
    QCOMPARE(tabs.currentEditor()->lineWrapMode(), QPlainTextEdit::NoWrap); // and on the editor
    QCOMPARE(tabs.currentEditor()->font().pointSizeF(), 14.0 * 140 / 100);
}

void ViewSettingsPersistenceTest::settingsAreAppliedToTabsOpenedLater()
{
    writeIni("[view]\nwordWrap=false\nzoom=120\n");
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    const ViewSettingsPersistence persistence(tabs, settings, 30ms);

    tabs.addEditor();

    QCOMPARE(tabs.editorAt(1)->lineWrapMode(), QPlainTextEdit::NoWrap);
}

void ViewSettingsPersistenceTest::startingWithoutASavedFileLeavesTheDefaultsAndWritesNothing()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    const ViewSettingsPersistence persistence(tabs, settings, 30ms);

    QCOMPARE(tabs.viewSettings(), ViewSettings());
    QTest::qWait(150); // longer than the save delay
    QVERIFY(!QFileInfo::exists(iniPath()));
    QVERIFY(!persistence.hasPendingChanges());
}

void ViewSettingsPersistenceTest::loadingDoesNotRewriteTheFile()
{
    writeIni("[view]\nzoom=150\n");
    const QDateTime before = QFileInfo(iniPath()).lastModified();
    SettingsManager settings(iniPath());
    EditorTabs tabs;

    QThread::msleep(80);
    const ViewSettingsPersistence persistence(tabs, settings, 30ms);
    QTest::qWait(150);

    QCOMPARE(QFileInfo(iniPath()).lastModified(), before);
    QCOMPARE(iniText(), QStringLiteral("[view]\nzoom=150\n"));
}

void ViewSettingsPersistenceTest::aChangeIsSavedAfterAShortWait()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    const ViewSettingsPersistence persistence(tabs, settings, 30ms);

    tabs.setZoomPercent(160);

    QTRY_VERIFY(iniText().contains(QStringLiteral("zoom=160")));
}

void ViewSettingsPersistenceTest::aChangeIsNotWrittenImmediately()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    const ViewSettingsPersistence persistence(tabs, settings, 5s); // a long wait

    tabs.setZoomPercent(160);

    QVERIFY(persistence.hasPendingChanges());
    QVERIFY(!QFileInfo::exists(iniPath())); // still only waiting
}

void ViewSettingsPersistenceTest::aBurstOfChangesIsSavedOnceWithTheFinalValue()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    const ViewSettingsPersistence persistence(tabs, settings, 80ms);

    // Ten wheel notches in quick succession.
    for (int i = 0; i < 10; ++i) {
        tabs.zoomBy(1);
        QTest::qWait(5); // shorter than the delay, so each change restarts the wait
    }
    QVERIFY2(!iniText().contains(QStringLiteral("zoom")), "nothing may be written mid-burst");

    QTRY_VERIFY(iniText().contains(QStringLiteral("zoom=200")));
    QVERIFY(!persistence.hasPendingChanges());
}

void ViewSettingsPersistenceTest::flushWritesAtOnce()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    ViewSettingsPersistence persistence(tabs, settings, 5s);
    tabs.setZoomPercent(130);
    QVERIFY(!QFileInfo::exists(iniPath()));

    persistence.flush();

    QVERIFY(iniText().contains(QStringLiteral("zoom=130")));
    QVERIFY(!persistence.hasPendingChanges());
}

void ViewSettingsPersistenceTest::destroyingItWritesWhatIsStillWaiting()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    {
        const ViewSettingsPersistence persistence(tabs, settings, 5s);
        tabs.setZoomPercent(170);
        tabs.setWordWrap(false);
        QVERIFY(!QFileInfo::exists(iniPath()));
    } // the window closing

    const QString text = iniText();
    QVERIFY2(text.contains(QStringLiteral("zoom=170")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("wordWrap=false")), qPrintable(text));
}

void ViewSettingsPersistenceTest::nothingIsPendingAfterASave()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    ViewSettingsPersistence persistence(tabs, settings, 5s);
    persistence.flush(); // nothing to write: harmless
    QVERIFY(!persistence.hasPendingChanges());
    QVERIFY(!QFileInfo::exists(iniPath()));

    tabs.setZoomPercent(120);
    persistence.flush();
    persistence.flush(); // a second flush finds nothing waiting
    QVERIFY(!persistence.hasPendingChanges());
}

void ViewSettingsPersistenceTest::everyKindOfChangeIsSaved()
{
    SettingsManager settings(iniPath());
    EditorTabs tabs;
    ViewSettingsPersistence persistence(tabs, settings, 5s);

    tabs.setWordWrap(false);
    persistence.flush();
    QVERIFY(iniText().contains(QStringLiteral("wordWrap=false")));

    QFont font(installedFamily());
    font.setPointSizeF(15.0);
    font.setBold(true);
    tabs.setBaseFont(font);
    persistence.flush();
    QVERIFY(iniText().contains(QStringLiteral("fontSize=15")));
    QVERIFY(iniText().contains(QStringLiteral("fontWeight=700")));

    tabs.setZoomPercent(250);
    persistence.flush();
    QVERIFY(iniText().contains(QStringLiteral("zoom=250")));
}

void ViewSettingsPersistenceTest::theSavedFileLoadsBackToTheSameSettings()
{
    ViewSettings expected;
    {
        SettingsManager settings(iniPath());
        EditorTabs tabs;
        ViewSettingsPersistence persistence(tabs, settings, 5s);
        QFont font(installedFamily());
        font.setPointSizeF(13.5);
        font.setItalic(true);
        tabs.setBaseFont(font);
        tabs.setWordWrap(false);
        tabs.setZoomPercent(180);
        expected = tabs.viewSettings();
    } // saved on destruction

    SettingsManager settings(iniPath());
    EditorTabs second;
    const ViewSettingsPersistence persistence(second, settings, 30ms);

    QCOMPARE(second.viewSettings().wordWrap, expected.wordWrap);
    QCOMPARE(second.viewSettings().zoomPercent, expected.zoomPercent);
    QCOMPARE(second.viewSettings().font.family(), expected.font.family());
    QCOMPARE(second.viewSettings().font.pointSizeF(), expected.font.pointSizeF());
    QCOMPARE(second.viewSettings().font.italic(), expected.font.italic());
}

void ViewSettingsPersistenceTest::changesBeforeTheTabsAreDestroyedAreStillSaved()
{
    // The window destroys its tabs before the persistence object; the pending copy must suffice.
    SettingsManager settings(iniPath());
    auto tabs = std::make_unique<EditorTabs>();
    ViewSettingsPersistence persistence(*tabs, settings, 5s);
    tabs->setZoomPercent(190);

    tabs.reset(); // the tabs are gone; nothing may touch them any more

    persistence.flush();
    QVERIFY(iniText().contains(QStringLiteral("zoom=190")));
}

void ViewSettingsPersistenceTest::anUnwritableLocationDoesNotCrash()
{
    QFile blocker(m_dir->filePath(QStringLiteral("blocker")));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();

    SettingsManager settings(m_dir->filePath(QStringLiteral("blocker/sub/scribe.ini")));
    EditorTabs tabs;
    ViewSettingsPersistence persistence(tabs, settings, 30ms);

    tabs.setZoomPercent(150);
    persistence.flush();

    QCOMPARE(tabs.viewSettings().zoomPercent, 150); // the program carries on regardless
    QVERIFY(!settings.isHealthy());
}

QTEST_MAIN(ViewSettingsPersistenceTest)

#include "ViewSettingsPersistenceTest.moc"
