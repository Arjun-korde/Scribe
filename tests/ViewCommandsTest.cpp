#include "ViewCommands.h"

#include "CommandRegistry.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QWidget>
#include <QtTest>

using scribe::core::CommandRegistry;
using scribe::editor::EditorTabs;
using scribe::editor::ViewCommands;
using scribe::editor::ViewSettings;

class ViewCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void registersSixCommandsWithTheirShortcuts();
    void zoomInHasAnAlternateShortcutForKeyboardsWithoutAPlusKey();

    void wordWrapTogglesAndReportsItsCheckState();
    void zoomCommandsChangeTheZoomInTenPercentSteps();
    void zoomCommandsAreUnavailableAtTheLimits();
    void restoreDefaultZoomIsUnavailableAtOneHundred();
    void fontCommandAppliesTheChosenFont();
    void fontCommandStartsFromTheUnzoomedFont();
    void cancellingTheFontDialogChangesNothing();
    void fontCommandIsUnavailableWithoutADialog();
    void statusBarCommandShowsAndHidesTheBar();

private:
    // A registry wired to real tabs, with a font dialog the test controls.
    struct Fixture
    {
        EditorTabs tabs;
        CommandRegistry registry;
        std::optional<QFont> answer;
        QFont askedWith;
        int asked = 0;

        explicit Fixture(bool withDialog = true)
        {
            ViewCommands::FontChooser chooser;
            if (withDialog) {
                chooser = [this](const QFont& current) {
                    ++asked;
                    askedWith = current;
                    return answer;
                };
            }
            ViewCommands::registerIn(registry, tabs, chooser);
        }

        bool run(QLatin1String id) const { return registry.run(id); }
        bool enabled(QLatin1String id) const { return registry.isEnabled(id); }
        bool checked(QLatin1String id) const { return registry.isChecked(id); }
        int zoom() const { return tabs.viewSettings().zoomPercent; }
    };
};

void ViewCommandsTest::registersSixCommandsWithTheirShortcuts()
{
    Fixture f;
    QCOMPARE(f.registry.commands().size(), size_t{6});

    const auto shortcut = [&](QLatin1String id) { return f.registry.find(id)->shortcut; };
    QCOMPARE(shortcut(ViewCommands::kZoomIn), QStringLiteral("Ctrl++"));
    QCOMPARE(shortcut(ViewCommands::kZoomOut), QStringLiteral("Ctrl+-"));
    QCOMPARE(shortcut(ViewCommands::kZoomReset), QStringLiteral("Ctrl+0"));
    QVERIFY(shortcut(ViewCommands::kWordWrap).isEmpty());
    QVERIFY(shortcut(ViewCommands::kFont).isEmpty());
    QVERIFY(shortcut(ViewCommands::kStatusBar).isEmpty());
}

void ViewCommandsTest::zoomInHasAnAlternateShortcutForKeyboardsWithoutAPlusKey()
{
    Fixture f;
    QCOMPARE(f.registry.find(ViewCommands::kZoomIn)->alternateShortcut, QStringLiteral("Ctrl+="));
}

void ViewCommandsTest::wordWrapTogglesAndReportsItsCheckState()
{
    Fixture f;
    QVERIFY(f.checked(ViewCommands::kWordWrap)); // on by default
    QCOMPARE(f.tabs.currentEditor()->lineWrapMode(), QPlainTextEdit::WidgetWidth);

    QVERIFY(f.run(ViewCommands::kWordWrap));
    QVERIFY(!f.checked(ViewCommands::kWordWrap));
    QCOMPARE(f.tabs.currentEditor()->lineWrapMode(), QPlainTextEdit::NoWrap);

    QVERIFY(f.run(ViewCommands::kWordWrap));
    QVERIFY(f.checked(ViewCommands::kWordWrap));
}

void ViewCommandsTest::zoomCommandsChangeTheZoomInTenPercentSteps()
{
    Fixture f;

    QVERIFY(f.run(ViewCommands::kZoomIn));
    QCOMPARE(f.zoom(), 110);
    QVERIFY(f.run(ViewCommands::kZoomIn));
    QCOMPARE(f.zoom(), 120);
    QVERIFY(f.run(ViewCommands::kZoomOut));
    QCOMPARE(f.zoom(), 110);
    QVERIFY(f.run(ViewCommands::kZoomReset));
    QCOMPARE(f.zoom(), 100);
}

void ViewCommandsTest::zoomCommandsAreUnavailableAtTheLimits()
{
    Fixture f;

    f.tabs.setZoomPercent(500);
    QVERIFY(!f.enabled(ViewCommands::kZoomIn));
    QVERIFY(!f.run(ViewCommands::kZoomIn));
    QVERIFY(f.enabled(ViewCommands::kZoomOut));

    f.tabs.setZoomPercent(10);
    QVERIFY(!f.enabled(ViewCommands::kZoomOut));
    QVERIFY(!f.run(ViewCommands::kZoomOut));
    QVERIFY(f.enabled(ViewCommands::kZoomIn));
    QCOMPARE(f.zoom(), 10);
}

void ViewCommandsTest::restoreDefaultZoomIsUnavailableAtOneHundred()
{
    Fixture f;
    QVERIFY(!f.enabled(ViewCommands::kZoomReset));

    f.tabs.zoomBy(2);
    QVERIFY(f.enabled(ViewCommands::kZoomReset));

    f.tabs.zoomBy(-2);
    QVERIFY(!f.enabled(ViewCommands::kZoomReset));
}

void ViewCommandsTest::fontCommandAppliesTheChosenFont()
{
    Fixture f;
    QFont chosen(QStringLiteral("Courier New"));
    chosen.setPointSizeF(16.0);
    f.answer = chosen;

    QVERIFY(f.run(ViewCommands::kFont));

    QCOMPARE(f.asked, 1);
    QCOMPARE(f.tabs.viewSettings().font.family(), QStringLiteral("Courier New"));
    QCOMPARE(f.tabs.viewSettings().font.pointSizeF(), 16.0);
    QCOMPARE(f.tabs.currentEditor()->font().family(), QStringLiteral("Courier New"));
}

void ViewCommandsTest::fontCommandStartsFromTheUnzoomedFont()
{
    Fixture f;
    f.tabs.setZoomPercent(200);
    const QFont base = f.tabs.viewSettings().font;
    f.answer = std::nullopt;

    QVERIFY(f.run(ViewCommands::kFont));

    // At 200% the editor shows a doubled size, but the dialog must offer the real font size.
    QCOMPARE(f.askedWith.pointSizeF(), base.pointSizeF());
    QCOMPARE(f.askedWith.family(), base.family());
}

void ViewCommandsTest::cancellingTheFontDialogChangesNothing()
{
    Fixture f;
    const ViewSettings before = f.tabs.viewSettings();
    f.answer = std::nullopt;

    QVERIFY(f.run(ViewCommands::kFont)); // the command ran; the user just said no

    QCOMPARE(f.asked, 1);
    QCOMPARE(f.tabs.viewSettings(), before);
}

void ViewCommandsTest::fontCommandIsUnavailableWithoutADialog()
{
    Fixture f(/*withDialog=*/false);
    QVERIFY(!f.enabled(ViewCommands::kFont));
    QVERIFY(!f.run(ViewCommands::kFont));
}

void ViewCommandsTest::statusBarCommandShowsAndHidesTheBar()
{
    Fixture f;
    QVERIFY(f.checked(ViewCommands::kStatusBar)); // shown by default
    QVERIFY(f.tabs.viewSettings().statusBarVisible);

    QVERIFY(f.run(ViewCommands::kStatusBar));
    QVERIFY(!f.tabs.viewSettings().statusBarVisible); // the setting; the window hides the widget
    QVERIFY(!f.checked(ViewCommands::kStatusBar));

    QVERIFY(f.run(ViewCommands::kStatusBar));
    QVERIFY(f.tabs.viewSettings().statusBarVisible);
    QVERIFY(f.checked(ViewCommands::kStatusBar));
}

QTEST_MAIN(ViewCommandsTest)

#include "ViewCommandsTest.moc"
