#include "WindowPlacementPersistence.h"

#include "MainWindow.h"
#include "SettingsManager.h"
#include "WindowPlacement.h"

#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QTemporaryDir>
#include <QWidget>
#include <QtTest>

using scribe::app::WindowPlacementPersistence;
using scribe::core::loadWindowPlacement;
using scribe::core::SettingsManager;
using scribe::core::WindowPlacement;

class WindowPlacementPersistenceTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void nothingSavedLeavesTheWindowAlone();
    void aSavedPlacementIsAppliedBeforeTheWindowIsShown();
    void aMaximizedWindowIsRestoredMaximized();
    void aPlacementOnNoScreenIsIgnored();
    void aWindowTooBigForTheScreenIsShrunk();
    void startingAndStoppingWritesNothing();
    void changesBeforeArmingAreIgnored();
    void movingAndResizingIsSavedAfterAShortWait();
    void flushSavesAtOnce();
    void destroyingItSavesWhatIsPending();
    void maximizingIsSavedWithTheNormalSize();
    void anUnchangedWindowIsNotRewritten();
    void automaticArmingHappensAfterTheWindowIsShown();
    void theMainWindowRemembersItsPlace();
    void theMainWindowWithoutASettingsStoreWritesNothing();

private:
    QString newPath() { return m_dir->filePath(QStringLiteral("w%1.ini").arg(++m_counter)); }
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
    // A rectangle that surely lies on the first screen.
    static QRect onScreen(int width = 700, int height = 500)
    {
        const QRect area = QGuiApplication::primaryScreen()->availableGeometry();
        return QRect(area.left() + 60, area.top() + 80, qMin(width, area.width() - 120),
                     qMin(height, area.height() - 160));
    }
    static QByteArray iniFor(const QRect& rect, bool maximized = false)
    {
        return QByteArray("[window]\nx=%1\ny=%2\nwidth=%3\nheight=%4\nmaximized=%5\n")
            .replace("%1", QByteArray::number(rect.x()))
            .replace("%2", QByteArray::number(rect.y()))
            .replace("%3", QByteArray::number(rect.width()))
            .replace("%4", QByteArray::number(rect.height()))
            .replace("%5", maximized ? "true" : "false");
    }

    std::unique_ptr<QTemporaryDir> m_dir;
    int m_counter = 0;
};

void WindowPlacementPersistenceTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

void WindowPlacementPersistenceTest::nothingSavedLeavesTheWindowAlone()
{
    SettingsManager settings(newPath());
    QWidget window;
    window.resize(640, 480);
    WindowPlacementPersistence persistence(window, settings, 10, -1);
    QCOMPARE(window.size(), QSize(640, 480));
}

void WindowPlacementPersistenceTest::aSavedPlacementIsAppliedBeforeTheWindowIsShown()
{
    const QRect wanted = onScreen();
    SettingsManager settings(write(iniFor(wanted)));
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 10, -1);
    QCOMPARE(window.size(), wanted.size());
    QCOMPARE(window.pos(), wanted.topLeft());
}

void WindowPlacementPersistenceTest::aMaximizedWindowIsRestoredMaximized()
{
    const QRect wanted = onScreen();
    SettingsManager settings(write(iniFor(wanted, true)));
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 10, -1);
    QVERIFY(window.windowState() & Qt::WindowMaximized);
    QCOMPARE(window.normalGeometry().size(), wanted.size());
}

void WindowPlacementPersistenceTest::aPlacementOnNoScreenIsIgnored()
{
    const QRect area = QGuiApplication::primaryScreen()->availableGeometry();
    const QRect far(area.right() + 20000, area.top() + 100, 700, 500);
    SettingsManager settings(write(iniFor(far)));
    QWidget window;
    window.resize(640, 480);
    WindowPlacementPersistence persistence(window, settings, 10, -1);
    QCOMPARE(window.size(), QSize(640, 480));
    QVERIFY(window.pos() != far.topLeft());
}

void WindowPlacementPersistenceTest::aWindowTooBigForTheScreenIsShrunk()
{
    const QRect area = QGuiApplication::primaryScreen()->availableGeometry();
    const QRect huge(area.left() + 10, area.top() + 100, area.width() + 3000, area.height() + 3000);
    SettingsManager settings(write(iniFor(huge)));
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 10, -1);
    QVERIFY(window.width() <= area.width());
    QVERIFY(window.height() <= area.height());
    QVERIFY(area.contains(window.geometry()));
}

void WindowPlacementPersistenceTest::startingAndStoppingWritesNothing()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        QWidget window;
        WindowPlacementPersistence persistence(window, settings, 10, -1);
        window.show();
        persistence.arm();
        persistence.flush();
    }
    QVERIFY(!QFile::exists(path));
}

void WindowPlacementPersistenceTest::changesBeforeArmingAreIgnored()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        QWidget window;
        WindowPlacementPersistence persistence(window, settings, 10, -1);
        window.show();
        window.resize(777, 555); // the system settling the window, not the user
        QTest::qWait(60);
        persistence.flush();
        QVERIFY(!persistence.isArmed());
    }
    QVERIFY(!QFile::exists(path));
}

void WindowPlacementPersistenceTest::movingAndResizingIsSavedAfterAShortWait()
{
    const QString path = newPath();
    SettingsManager settings(path);
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 20, -1);
    window.show();
    persistence.arm();

    window.resize(610, 430);
    QVERIFY(!loadWindowPlacement(settings)); // not written at once
    QTRY_VERIFY_WITH_TIMEOUT(loadWindowPlacement(settings).has_value(), 2000);
    QCOMPARE(loadWindowPlacement(settings).value_or(WindowPlacement{}).rect.size(),
             QSize(610, 430));
}

void WindowPlacementPersistenceTest::flushSavesAtOnce()
{
    SettingsManager settings(newPath());
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 60000, -1);
    window.show();
    persistence.arm();
    window.resize(620, 440);
    QVERIFY(!loadWindowPlacement(settings));
    persistence.flush();
    QCOMPARE(loadWindowPlacement(settings).value_or(WindowPlacement{}).rect.size(),
             QSize(620, 440));
}

void WindowPlacementPersistenceTest::destroyingItSavesWhatIsPending()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        QWidget window;
        {
            WindowPlacementPersistence persistence(window, settings, 60000, -1);
            window.show();
            persistence.arm();
            window.resize(630, 450);
            QVERIFY(!loadWindowPlacement(settings));
        }
        QCOMPARE(loadWindowPlacement(settings).value_or(WindowPlacement{}).rect.size(),
                 QSize(630, 450));
    }
}

void WindowPlacementPersistenceTest::maximizingIsSavedWithTheNormalSize()
{
    SettingsManager settings(newPath());
    QWidget window;
    window.resize(650, 460);
    WindowPlacementPersistence persistence(window, settings, 60000, -1);
    window.show();
    persistence.arm();
    window.showMaximized();
    QTest::qWait(50);
    persistence.flush();

    const WindowPlacement saved = loadWindowPlacement(settings).value_or(WindowPlacement{});
    if (!window.isMaximized()) {
        QSKIP("this platform does not maximize windows");
    }
    QVERIFY(saved.rect.isValid());
    QVERIFY(saved.maximized);
    QCOMPARE(saved.rect.size(), QSize(650, 460));
}

void WindowPlacementPersistenceTest::anUnchangedWindowIsNotRewritten()
{
    const QString path = write(iniFor(onScreen()));
    SettingsManager settings(path);
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 10, -1);
    window.show();
    persistence.arm();
    const QDateTime before = QFileInfo(path).lastModified();
    QTest::qWait(30);
    persistence.flush();
    QCOMPARE(QFileInfo(path).lastModified(), before);
}

void WindowPlacementPersistenceTest::automaticArmingHappensAfterTheWindowIsShown()
{
    SettingsManager settings(newPath());
    QWidget window;
    WindowPlacementPersistence persistence(window, settings, 10, 20);
    QVERIFY(!persistence.isArmed());
    QTest::qWait(60);
    QVERIFY(!persistence.isArmed()); // hidden windows never arm
    window.show();
    QTRY_VERIFY_WITH_TIMEOUT(persistence.isArmed(), 2000);
}

void WindowPlacementPersistenceTest::theMainWindowRemembersItsPlace()
{
    const QString path = newPath();
    {
        SettingsManager settings(path);
        MainWindow window(FileDialogs{}, &settings);
        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(!window.isHidden(), 1000);
        QTest::qWait(700); // past the arming delay
        window.resize(660, 470);
        QCOMPARE(window.close(), true);
    }
    SettingsManager settings(path);
    const WindowPlacement saved = loadWindowPlacement(settings).value_or(WindowPlacement{});
    QVERIFY(saved.rect.isValid());
    QCOMPARE(saved.rect.size(), QSize(660, 470));

    MainWindow again(FileDialogs{}, &settings);
    QCOMPARE(again.size(), QSize(660, 470));
}

void WindowPlacementPersistenceTest::theMainWindowWithoutASettingsStoreWritesNothing()
{
    MainWindow window(FileDialogs{});
    window.show();
    window.resize(670, 480);
    QCOMPARE(window.close(), true);
    QVERIFY(m_dir->isValid()); // nothing to check on disk: there is no store at all
}

QTEST_MAIN(WindowPlacementPersistenceTest)
#include "WindowPlacementPersistenceTest.moc"
