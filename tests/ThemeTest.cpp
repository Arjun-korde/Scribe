#include "Theme.h"
#include "ThemeManager.h"

#include <QtTest>

using scribe::app::Theme;
using scribe::app::ThemeManager;

class ThemeTest : public QObject
{
    Q_OBJECT

private slots:
    void lightMatchesFigmaTokens()
    {
        const Theme t = Theme::light();
        QCOMPARE(t.surface, QColor("#ffffff"));
        QCOMPARE(t.surfaceChrome, QColor("#f3f3f3"));
        QCOMPARE(t.textPrimary, QColor("#1b1b1b"));
        QCOMPARE(t.textMuted, QColor("#5c5c5c"));
        QCOMPARE(t.outline, QColor("#e0e0e0"));
        QCOMPARE(t.currentLine, QColor("#f5f5f5"));
        QCOMPARE(t.selection, QColor("#cce4f7"));
        QCOMPARE(t.accent, QColor("#0f6cbd"));
    }

    void darkMatchesFigmaTokens()
    {
        const Theme t = Theme::dark();
        QCOMPARE(t.surface, QColor("#1e1e1e"));
        QCOMPARE(t.surfaceChrome, QColor("#2b2b2b"));
        QCOMPARE(t.textPrimary, QColor("#e6e6e6"));
        QCOMPARE(t.textMuted, QColor("#a0a0a0"));
        QCOMPARE(t.outline, QColor("#3a3a3a"));
        QCOMPARE(t.currentLine, QColor("#262626"));
        QCOMPARE(t.selection, QColor("#264f78"));
        QCOMPARE(t.accent, QColor("#60cdff"));
    }

    void paletteMapsTokensToRoles()
    {
        const Theme t = Theme::dark();
        const QPalette p = t.palette();
        QCOMPARE(p.color(QPalette::Base), t.surface);
        QCOMPARE(p.color(QPalette::Window), t.surfaceChrome);
        QCOMPARE(p.color(QPalette::Text), t.textPrimary);
        QCOMPARE(p.color(QPalette::Highlight), t.selection);
        QCOMPARE(p.color(QPalette::HighlightedText), t.textPrimary);
        QCOMPARE(p.color(QPalette::Disabled, QPalette::Text), t.textMuted);
    }

    void styleSheetUsesTokensAndLeavesNoPlaceholders()
    {
        const Theme t = Theme::light();
        const QString css = t.styleSheet();
        QVERIFY(css.contains("#f3f3f3")); // surfaceChrome
        QVERIFY(css.contains("#e0e0e0")); // outline
        QVERIFY(css.contains("#cce4f7")); // selection
        QVERIFY(!css.contains("%1"));
        QVERIFY(!css.contains("%6"));
    }

    void colorSchemePicksTheme()
    {
        QCOMPARE(ThemeManager::themeFor(Qt::ColorScheme::Dark).name, Theme::dark().name);
        QCOMPARE(ThemeManager::themeFor(Qt::ColorScheme::Light).name, Theme::light().name);
        QCOMPARE(ThemeManager::themeFor(Qt::ColorScheme::Unknown).name, Theme::light().name);
    }

    void managerAppliesThemeToApplication()
    {
        auto* app = qobject_cast<QApplication*>(QApplication::instance());
        QVERIFY(app != nullptr);
        ThemeManager manager(*app);
        QCOMPARE(app->palette().color(QPalette::Base), manager.theme().surface);
        QVERIFY(!app->styleSheet().isEmpty());
    }
};

QTEST_MAIN(ThemeTest)
#include "ThemeTest.moc"
