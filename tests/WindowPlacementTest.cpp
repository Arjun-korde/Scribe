#include "WindowPlacement.h"

#include "SettingsManager.h"

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

using scribe::core::fitToScreens;
using scribe::core::loadWindowPlacement;
using scribe::core::saveWindowPlacement;
using scribe::core::SettingsManager;
using scribe::core::WindowPlacement;

namespace
{

constexpr int kTitleBar = 30;

// A typical desktop: the usable area of a 1920 x 1080 screen without its taskbar.
const QRect kScreen(0, 0, 1920, 1040);

QList<QRect> oneScreen()
{
    return {kScreen};
}

// Two monitors side by side.
QList<QRect> twoScreens()
{
    return {kScreen, QRect(1920, 0, 1920, 1040)};
}

// A second monitor to the left of the main one, so positions there are negative.
QList<QRect> leftAndMain()
{
    return {kScreen, QRect(-1920, 0, 1920, 1040)};
}

} // namespace

// ---- fitToScreens ---------------------------------------------------------------------------

TEST(FitToScreens, AWindowFullyOnAScreenIsLeftAlone)
{
    EXPECT_EQ(fitToScreens(QRect(100, 100, 900, 600), oneScreen(), kTitleBar),
              QRect(100, 100, 900, 600));
}

TEST(FitToScreens, AWindowFillingTheScreenBelowItsTitleBarIsLeftAlone)
{
    const QRect full(0, kTitleBar, 1920, 1040 - kTitleBar);
    EXPECT_EQ(fitToScreens(full, oneScreen(), kTitleBar), full);
}

TEST(FitToScreens, AWindowPartlyOffTheRightEdgeIsMovedIn)
{
    EXPECT_EQ(fitToScreens(QRect(1500, 100, 900, 600), oneScreen(), kTitleBar),
              QRect(1020, 100, 900, 600));
}

TEST(FitToScreens, AWindowPartlyOffTheBottomIsMovedUp)
{
    EXPECT_EQ(fitToScreens(QRect(100, 800, 900, 600), oneScreen(), kTitleBar),
              QRect(100, 440, 900, 600));
}

TEST(FitToScreens, AWindowPartlyOffTheLeftEdgeIsMovedIn)
{
    EXPECT_EQ(fitToScreens(QRect(-300, 100, 900, 600), oneScreen(), kTitleBar),
              QRect(0, 100, 900, 600));
}

TEST(FitToScreens, TheTitleBarIsKeptOnScreenWhenTheWindowIsAtTheTop)
{
    // A client area at y=0 puts the title bar above the screen, where it cannot be grabbed.
    EXPECT_EQ(fitToScreens(QRect(100, 0, 900, 600), oneScreen(), kTitleBar),
              QRect(100, kTitleBar, 900, 600));
    EXPECT_EQ(fitToScreens(QRect(100, -200, 900, 600), oneScreen(), kTitleBar),
              QRect(100, kTitleBar, 900, 600));
}

TEST(FitToScreens, AWindowBiggerThanTheScreenIsShrunkToFit)
{
    EXPECT_EQ(fitToScreens(QRect(0, 0, 3000, 2000), oneScreen(), kTitleBar),
              QRect(0, kTitleBar, 1920, 1040 - kTitleBar));
}

TEST(FitToScreens, OnlyTheOversizedDimensionIsShrunk)
{
    EXPECT_EQ(fitToScreens(QRect(100, 100, 3000, 400), oneScreen(), kTitleBar),
              QRect(0, 100, 1920, 400));
    EXPECT_EQ(fitToScreens(QRect(100, 100, 500, 5000), oneScreen(), kTitleBar),
              QRect(100, kTitleBar, 500, 1040 - kTitleBar));
}

TEST(FitToScreens, ASmallerScreenThanBeforeShrinksAndMovesTheWindowIn)
{
    const QList<QRect> laptop = {QRect(0, 0, 1366, 728)};
    EXPECT_EQ(fitToScreens(QRect(800, 300, 900, 600), laptop, kTitleBar),
              QRect(466, 128, 900, 600));
}

TEST(FitToScreens, AWindowOnASecondMonitorStaysThereWhileItIsConnected)
{
    EXPECT_EQ(fitToScreens(QRect(2000, 100, 900, 600), twoScreens(), kTitleBar),
              QRect(2000, 100, 900, 600));
}

TEST(FitToScreens, AWindowOnAnUnpluggedMonitorGivesNothing)
{
    EXPECT_EQ(fitToScreens(QRect(2000, 100, 900, 600), oneScreen(), kTitleBar), std::nullopt);
    EXPECT_EQ(fitToScreens(QRect(5000, 100, 900, 600), twoScreens(), kTitleBar), std::nullopt);
}

TEST(FitToScreens, MonitorsToTheLeftOrAboveUseNegativeCoordinates)
{
    EXPECT_EQ(fitToScreens(QRect(-1800, 100, 900, 600), leftAndMain(), kTitleBar),
              QRect(-1800, 100, 900, 600));
    // The same position is useless once that monitor is gone.
    EXPECT_EQ(fitToScreens(QRect(-1800, 100, 900, 600), oneScreen(), kTitleBar), std::nullopt);

    const QList<QRect> above = {kScreen, QRect(0, -1040, 1920, 1040)};
    EXPECT_EQ(fitToScreens(QRect(100, -900, 900, 600), above, kTitleBar),
              QRect(100, -900, 900, 600));
    EXPECT_EQ(fitToScreens(QRect(100, -900, 900, 600), oneScreen(), kTitleBar), std::nullopt);
}

TEST(FitToScreens, AWindowStraddlingTwoMonitorsGoesToTheOneHoldingMoreOfIt)
{
    // 420 px on the first monitor, 480 px on the second: the second wins.
    EXPECT_EQ(fitToScreens(QRect(1500, 100, 900, 600), twoScreens(), kTitleBar),
              QRect(1920, 100, 900, 600));
}

TEST(FitToScreens, ATieGoesToTheFirstScreenInTheList)
{
    EXPECT_EQ(fitToScreens(QRect(1470, 100, 900, 600), twoScreens(), kTitleBar),
              QRect(1020, 100, 900, 600));
}

TEST(FitToScreens, AThinSliverOnAScreenIsBroughtFullyIntoView)
{
    // Only 20 px of the window is still on the screen; it is pulled in, not thrown away.
    EXPECT_EQ(fitToScreens(QRect(1900, 100, 900, 600), oneScreen(), kTitleBar),
              QRect(1020, 100, 900, 600));
}

TEST(FitToScreens, AWindowOnlyJustAboveTheScreenIsStillRescued)
{
    EXPECT_EQ(fitToScreens(QRect(100, -590, 900, 600), oneScreen(), kTitleBar),
              QRect(100, kTitleBar, 900, 600));
}

TEST(FitToScreens, ScreensOfDifferentSizesAreHandledOneByOne)
{
    const QList<QRect> mixed = {QRect(0, 0, 1366, 728), QRect(1366, 0, 2560, 1400)};
    EXPECT_EQ(fitToScreens(QRect(2000, 100, 1500, 1000), mixed, kTitleBar),
              QRect(2000, 100, 1500, 1000)); // fits on the big monitor
    EXPECT_EQ(fitToScreens(QRect(100, 100, 1500, 1000), mixed, kTitleBar),
              QRect(0, kTitleBar, 1366, 698)); // on the small one it must shrink and move up
}

TEST(FitToScreens, InvalidInputGivesNothing)
{
    EXPECT_EQ(fitToScreens(QRect(), oneScreen(), kTitleBar), std::nullopt);
    EXPECT_EQ(fitToScreens(QRect(10, 10, 0, 500), oneScreen(), kTitleBar), std::nullopt);
    EXPECT_EQ(fitToScreens(QRect(10, 10, 500, -5), oneScreen(), kTitleBar), std::nullopt);
    EXPECT_EQ(fitToScreens(QRect(10, 10, 500, 500), {}, kTitleBar), std::nullopt);
}

TEST(FitToScreens, WorksWithoutTitleBarInformation)
{
    EXPECT_EQ(fitToScreens(QRect(-200, -10, 900, 600), oneScreen(), 0), QRect(0, 0, 900, 600));
    EXPECT_EQ(fitToScreens(QRect(-200, -10, 900, 600), oneScreen(), -50), QRect(0, 0, 900, 600));
}

TEST(FitToScreens, ATinyScreenWithNoRoomForTheTitleBarGivesNothing)
{
    const QList<QRect> sliver = {QRect(0, 0, 800, 20)};
    EXPECT_EQ(fitToScreens(QRect(0, 0, 400, 300), sliver, kTitleBar), std::nullopt);
}

// ---- Properties checked on many windows -----------------------------------------------------

namespace
{

// A tiny deterministic generator, so the test needs no random library and never flakes.
class Lcg
{
public:
    explicit Lcg(unsigned seed)
        : m_state(seed)
    {}
    int next(int limit)
    {
        m_state = m_state * 1664525u + 1013904223u;
        return static_cast<int>((m_state >> 8u) % static_cast<unsigned>(limit));
    }

private:
    unsigned m_state;
};

bool fitsOnSomeScreen(const QRect& result, const QList<QRect>& screens)
{
    for (const QRect& screen : screens) {
        if (screen.contains(result) && result.top() - kTitleBar >= screen.top()) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST(FitToScreens, TheResultAlwaysFitsFullyOnAScreenWithItsTitleBar)
{
    Lcg random(4242);
    const QList<QList<QRect>> setups = {
        oneScreen(), twoScreens(), leftAndMain(), {QRect(0, 0, 1366, 728)}};
    for (const auto& screens : setups) {
        for (int i = 0; i < 2000; ++i) {
            const QRect wanted(random.next(8000) - 3000, random.next(4000) - 1500,
                               1 + random.next(4000), 1 + random.next(3000));
            const auto fitted = fitToScreens(wanted, screens, kTitleBar);
            if (fitted) {
                EXPECT_TRUE(fitsOnSomeScreen(*fitted, screens))
                    << wanted.x() << "," << wanted.y() << " " << wanted.width() << "x"
                    << wanted.height();
            }
        }
    }
}

TEST(FitToScreens, FittingTwiceGivesTheSameAsFittingOnce)
{
    Lcg random(99);
    for (int i = 0; i < 2000; ++i) {
        const QRect wanted(random.next(6000) - 2000, random.next(3000) - 1000,
                           1 + random.next(3500), 1 + random.next(2500));
        const auto once = fitToScreens(wanted, twoScreens(), kTitleBar);
        if (once) {
            EXPECT_EQ(fitToScreens(*once, twoScreens(), kTitleBar), once);
        }
    }
}

TEST(FitToScreens, AWindowThatAlreadyFitsIsNeverChanged)
{
    Lcg random(7);
    for (int i = 0; i < 2000; ++i) {
        const int width = 320 + random.next(1500);
        const int height = 200 + random.next(780);
        const QRect inside(random.next(1920 - width), kTitleBar + random.next(1010 - height), width,
                           height);
        EXPECT_EQ(fitToScreens(inside, oneScreen(), kTitleBar), inside);
    }
}

// ---- Saving and loading ---------------------------------------------------------------------

class WindowPlacementStoreTest : public testing::Test
{
protected:
    void SetUp() override { ASSERT_TRUE(m_dir.isValid()); }

    // A new file name for every file, because Qt caches parsed INI files and only re-reads one
    // when its size or timestamp changes (see the SettingsManager tests).
    QString writeIni(const QByteArray& bytes)
    {
        const QString path = m_dir.filePath(QStringLiteral("p%1.ini").arg(++m_counter));
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(bytes);
        return path;
    }

    QString emptyIni() { return m_dir.filePath(QStringLiteral("e%1.ini").arg(++m_counter)); }

    QTemporaryDir m_dir;
    int m_counter = 0;
};

TEST_F(WindowPlacementStoreTest, NothingSavedGivesNothing)
{
    const SettingsManager settings(emptyIni());
    EXPECT_EQ(loadWindowPlacement(settings), std::nullopt);
}

TEST_F(WindowPlacementStoreTest, ASavedPlacementIsLoadedAgain)
{
    const QString path = emptyIni();
    {
        SettingsManager settings(path);
        saveWindowPlacement(settings, {QRect(120, 80, 900, 600), false});
    }
    const SettingsManager settings(path);
    EXPECT_EQ(loadWindowPlacement(settings), (WindowPlacement{QRect(120, 80, 900, 600), false}));
}

TEST_F(WindowPlacementStoreTest, MaximizedAndNegativePositionsRoundTrip)
{
    const QString path = emptyIni();
    {
        SettingsManager settings(path);
        saveWindowPlacement(settings, {QRect(-1800, -40, 1024, 768), true});
    }
    const SettingsManager settings(path);
    EXPECT_EQ(loadWindowPlacement(settings), (WindowPlacement{QRect(-1800, -40, 1024, 768), true}));
}

TEST_F(WindowPlacementStoreTest, TheFileIsReadable)
{
    const QString path = emptyIni();
    {
        SettingsManager settings(path);
        saveWindowPlacement(settings, {QRect(120, 80, 900, 600), true});
    }
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    EXPECT_TRUE(text.contains(QStringLiteral("[window]"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("x=120"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("y=80"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("width=900"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("height=600"))) << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("maximized=true"))) << qPrintable(text);
}

TEST_F(WindowPlacementStoreTest, AHandEditedFileIsLoaded)
{
    const SettingsManager settings(
        writeIni("[window]\nx=10\ny=20\nwidth=800\nheight=500\nmaximized=true\n"));
    EXPECT_EQ(loadWindowPlacement(settings), (WindowPlacement{QRect(10, 20, 800, 500), true}));
}

TEST_F(WindowPlacementStoreTest, AnyMissingNumberMakesTheWholePlacementUnusable)
{
    const char* all[] = {"x=10\n", "y=20\n", "width=800\n", "height=500\n"};
    for (int skipped = 0; skipped < 4; ++skipped) {
        QByteArray text = "[window]\n";
        for (int i = 0; i < 4; ++i) {
            if (i != skipped) {
                text += all[i];
            }
        }
        const SettingsManager settings(writeIni(text));
        EXPECT_EQ(loadWindowPlacement(settings), std::nullopt) << "without " << all[skipped];
    }
}

TEST_F(WindowPlacementStoreTest, DamagedNumbersMakeThePlacementUnusable)
{
    for (const char* bad :
         {"x=banana\ny=20\nwidth=800\nheight=500\n", "x=10\ny=\nwidth=800\nheight=500\n",
          "x=10\ny=20\nwidth=800.5\nheight=500\n", "x=10\ny=20\nwidth=800\nheight=5e2\n"}) {
        const SettingsManager settings(writeIni(QByteArray("[window]\n") + bad));
        EXPECT_EQ(loadWindowPlacement(settings), std::nullopt) << bad;
    }
}

TEST_F(WindowPlacementStoreTest, AbsurdSizesAreRefused)
{
    for (const char* bad :
         {"x=0\ny=0\nwidth=100\nheight=500\n",      // narrower than a usable window
          "x=0\ny=0\nwidth=319\nheight=500\n",      // just under the minimum
          "x=0\ny=0\nwidth=800\nheight=199\n",      // just under the minimum
          "x=0\ny=0\nwidth=800\nheight=0\n",        //
          "x=0\ny=0\nwidth=-800\nheight=500\n",     //
          "x=0\ny=0\nwidth=40000\nheight=500\n",    // wider than any screen setup
          "x=0\ny=0\nwidth=800\nheight=99999999\n", //
          "x=999999\ny=0\nwidth=800\nheight=500\n", // positioned absurdly far away
          "x=0\ny=-999999\nwidth=800\nheight=500\n"}) {
        const SettingsManager settings(writeIni(QByteArray("[window]\n") + bad));
        EXPECT_EQ(loadWindowPlacement(settings), std::nullopt) << bad;
    }
}

TEST_F(WindowPlacementStoreTest, TheLimitsThemselvesAreAccepted)
{
    const SettingsManager minimum(writeIni("[window]\nx=-3840\ny=-100\nwidth=320\nheight=200\n"));
    EXPECT_EQ(loadWindowPlacement(minimum), (WindowPlacement{QRect(-3840, -100, 320, 200), false}));

    const SettingsManager large(writeIni("[window]\nx=0\ny=0\nwidth=30000\nheight=30000\n"));
    EXPECT_EQ(loadWindowPlacement(large), (WindowPlacement{QRect(0, 0, 30000, 30000), false}));
}

TEST_F(WindowPlacementStoreTest, ADamagedMaximizedFlagMeansNotMaximized)
{
    const SettingsManager settings(
        writeIni("[window]\nx=10\ny=20\nwidth=800\nheight=500\nmaximized=perhaps\n"));
    EXPECT_EQ(loadWindowPlacement(settings), (WindowPlacement{QRect(10, 20, 800, 500), false}));
}

TEST_F(WindowPlacementStoreTest, LoadingNeverCreatesTheFile)
{
    const QString path = emptyIni();
    {
        const SettingsManager settings(path);
        (void)loadWindowPlacement(settings);
    }
    EXPECT_FALSE(QFile::exists(path));
}

TEST_F(WindowPlacementStoreTest, SavingAnotherPlacementReplacesTheFirst)
{
    const QString path = emptyIni();
    {
        SettingsManager settings(path);
        saveWindowPlacement(settings, {QRect(10, 20, 800, 500), false});
        saveWindowPlacement(settings, {QRect(30, 40, 1000, 700), true});
    }
    const SettingsManager settings(path);
    EXPECT_EQ(loadWindowPlacement(settings), (WindowPlacement{QRect(30, 40, 1000, 700), true}));
}
