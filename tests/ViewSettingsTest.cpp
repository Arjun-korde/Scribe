#include "ViewSettings.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QApplication>
#include <QFontInfo>
#include <QSignalSpy>
#include <QWheelEvent>
#include <QtTest>

using scribe::editor::EditorTabs;
using scribe::editor::EditorWidget;
using scribe::editor::ViewSettings;

class ViewSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsMatchNotepad();
    void defaultFontIsFixedWidthAtElevenPoints();
    void zoomIsClampedToItsRange();
    void zoomedFontScalesThePointSize();
    void zoomedFontScalesAPixelSizedFont();

    void newTabsStartWithTheDefaults();
    void wordWrapReachesEveryTabIncludingNewOnes();
    void zoomReachesEveryTabIncludingNewOnes();
    void zoomStepsAreTenPercentAndStopAtTheLimits();
    void resetZoomGoesBackToOneHundred();
    void changingTheBaseFontKeepsTheZoom();
    void signalIsEmittedOnlyForRealChanges();
    void setViewSettingsAppliesEverythingAtOnce();
    void setViewSettingsClampsTheZoom();
    void setViewSettingsEmitsOnceAndOnlyForRealChanges();

    void ctrlWheelZoomsAllTabsInTenPercentSteps();
    void wheelWithoutCtrlDoesNotZoom();
    void smallTouchpadMovementsAddUpToANotch();

private:
    static void wheel(QWidget* target, int angleY, Qt::KeyboardModifiers modifiers);
};

void ViewSettingsTest::wheel(QWidget* target, int angleY, Qt::KeyboardModifiers modifiers)
{
    QWheelEvent event(QPointF(5, 5), target->mapToGlobal(QPointF(5, 5)), QPoint(),
                      QPoint(0, angleY), Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
    QApplication::sendEvent(target, &event);
}

void ViewSettingsTest::defaultsMatchNotepad()
{
    const ViewSettings settings;
    QVERIFY(settings.wordWrap); // Windows 11 Notepad wraps by default
    QCOMPARE(settings.zoomPercent, 100);
}

void ViewSettingsTest::defaultFontIsFixedWidthAtElevenPoints()
{
    const QFont font = ViewSettings::defaultFont();
    QVERIFY2(QFontInfo(font).fixedPitch(), qPrintable(font.family()));
    QCOMPARE(font.pointSize(), 11);
}

void ViewSettingsTest::zoomIsClampedToItsRange()
{
    QCOMPARE(ViewSettings::clampZoom(-50), 10);
    QCOMPARE(ViewSettings::clampZoom(0), 10);
    QCOMPARE(ViewSettings::clampZoom(10), 10);
    QCOMPARE(ViewSettings::clampZoom(250), 250);
    QCOMPARE(ViewSettings::clampZoom(500), 500);
    QCOMPARE(ViewSettings::clampZoom(100000), 500);
}

void ViewSettingsTest::zoomedFontScalesThePointSize()
{
    ViewSettings settings;
    settings.font = QFont(QStringLiteral("Consolas"));
    settings.font.setPointSizeF(10.0);

    settings.zoomPercent = 150;
    QCOMPARE(settings.zoomedFont().pointSizeF(), 15.0);
    settings.zoomPercent = 50;
    QCOMPARE(settings.zoomedFont().pointSizeF(), 5.0);
    settings.zoomPercent = 100;
    QCOMPARE(settings.zoomedFont().pointSizeF(), 10.0);
    QCOMPARE(settings.font.pointSizeF(), 10.0); // the base font itself is never changed
}

void ViewSettingsTest::zoomedFontScalesAPixelSizedFont()
{
    ViewSettings settings;
    settings.font = QFont(QStringLiteral("Consolas"));
    settings.font.setPixelSize(20);

    settings.zoomPercent = 200;
    QCOMPARE(settings.zoomedFont().pixelSize(), 40);
    settings.zoomPercent = 10;
    QCOMPARE(settings.zoomedFont().pixelSize(), 2);
}

void ViewSettingsTest::newTabsStartWithTheDefaults()
{
    EditorTabs tabs;
    const EditorWidget* editor = tabs.currentEditor();

    QCOMPARE(tabs.viewSettings(), ViewSettings());
    QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
    QCOMPARE(editor->font().pointSizeF(), tabs.viewSettings().zoomedFont().pointSizeF());
}

void ViewSettingsTest::wordWrapReachesEveryTabIncludingNewOnes()
{
    EditorTabs tabs;
    tabs.addEditor();

    tabs.setWordWrap(false);
    QCOMPARE(tabs.editorAt(0)->lineWrapMode(), QPlainTextEdit::NoWrap);
    QCOMPARE(tabs.editorAt(1)->lineWrapMode(), QPlainTextEdit::NoWrap);

    tabs.addEditor(); // a tab created later follows the shared setting
    QCOMPARE(tabs.editorAt(2)->lineWrapMode(), QPlainTextEdit::NoWrap);

    tabs.setWordWrap(true);
    for (int i = 0; i < tabs.count(); ++i) {
        QCOMPARE(tabs.editorAt(i)->lineWrapMode(), QPlainTextEdit::WidgetWidth);
    }
}

void ViewSettingsTest::zoomReachesEveryTabIncludingNewOnes()
{
    EditorTabs tabs;
    tabs.addEditor();
    const qreal base = tabs.viewSettings().font.pointSizeF();

    tabs.setZoomPercent(200);
    QCOMPARE(tabs.editorAt(0)->font().pointSizeF(), base * 2);
    QCOMPARE(tabs.editorAt(1)->font().pointSizeF(), base * 2);

    tabs.addEditor();
    QCOMPARE(tabs.editorAt(2)->font().pointSizeF(), base * 2);
}

void ViewSettingsTest::zoomStepsAreTenPercentAndStopAtTheLimits()
{
    EditorTabs tabs;

    tabs.zoomBy(1);
    QCOMPARE(tabs.viewSettings().zoomPercent, 110);
    tabs.zoomBy(3);
    QCOMPARE(tabs.viewSettings().zoomPercent, 140);
    tabs.zoomBy(-5);
    QCOMPARE(tabs.viewSettings().zoomPercent, 90);

    tabs.zoomBy(1000);
    QCOMPARE(tabs.viewSettings().zoomPercent, 500);
    tabs.zoomBy(-1000);
    QCOMPARE(tabs.viewSettings().zoomPercent, 10);
}

void ViewSettingsTest::resetZoomGoesBackToOneHundred()
{
    EditorTabs tabs;
    tabs.setZoomPercent(300);
    tabs.resetZoom();
    QCOMPARE(tabs.viewSettings().zoomPercent, 100);
}

void ViewSettingsTest::changingTheBaseFontKeepsTheZoom()
{
    EditorTabs tabs;
    tabs.setZoomPercent(200);

    QFont font(QStringLiteral("Courier New"));
    font.setPointSizeF(14.0);
    tabs.setBaseFont(font);

    QCOMPARE(tabs.viewSettings().zoomPercent, 200);
    QCOMPARE(tabs.viewSettings().font.pointSizeF(), 14.0); // the dialog sees the unzoomed size
    QCOMPARE(tabs.currentEditor()->font().pointSizeF(), 28.0);
    QCOMPARE(tabs.currentEditor()->font().family(), QStringLiteral("Courier New"));
}

void ViewSettingsTest::signalIsEmittedOnlyForRealChanges()
{
    EditorTabs tabs;
    QSignalSpy spy(&tabs, &EditorTabs::viewSettingsChanged);

    tabs.setWordWrap(true); // already on
    tabs.setZoomPercent(100);
    tabs.resetZoom();
    tabs.setBaseFont(tabs.viewSettings().font);
    QCOMPARE(spy.count(), 0);

    tabs.setWordWrap(false);
    QCOMPARE(spy.count(), 1);
    tabs.zoomBy(1);
    QCOMPARE(spy.count(), 2);
    tabs.setZoomPercent(10000); // clamps to 500
    QCOMPARE(spy.count(), 3);
    tabs.setZoomPercent(500); // already at the limit
    QCOMPARE(spy.count(), 3);
}

void ViewSettingsTest::ctrlWheelZoomsAllTabsInTenPercentSteps()
{
    EditorTabs tabs;
    tabs.addEditor();

    wheel(tabs.currentEditor()->viewport(), 120, Qt::ControlModifier); // one notch up
    QCOMPARE(tabs.viewSettings().zoomPercent, 110);
    wheel(tabs.currentEditor()->viewport(), 240, Qt::ControlModifier); // two notches
    QCOMPARE(tabs.viewSettings().zoomPercent, 130);
    wheel(tabs.currentEditor()->viewport(), -120, Qt::ControlModifier);
    QCOMPARE(tabs.viewSettings().zoomPercent, 120);

    // Both tabs follow, not only the one under the mouse.
    QCOMPARE(tabs.editorAt(0)->font().pointSizeF(), tabs.viewSettings().zoomedFont().pointSizeF());
    QCOMPARE(tabs.editorAt(1)->font().pointSizeF(), tabs.viewSettings().zoomedFont().pointSizeF());
}

void ViewSettingsTest::wheelWithoutCtrlDoesNotZoom()
{
    EditorTabs tabs;
    wheel(tabs.currentEditor()->viewport(), 120, Qt::NoModifier);
    wheel(tabs.currentEditor()->viewport(), -120, Qt::ShiftModifier);
    QCOMPARE(tabs.viewSettings().zoomPercent, 100);
}

void ViewSettingsTest::smallTouchpadMovementsAddUpToANotch()
{
    EditorTabs tabs;

    for (int i = 0; i < 5; ++i) {
        wheel(tabs.currentEditor()->viewport(), 20, Qt::ControlModifier); // 100 units: not yet
    }
    QCOMPARE(tabs.viewSettings().zoomPercent, 100);

    wheel(tabs.currentEditor()->viewport(), 20, Qt::ControlModifier); // 120 units: one notch
    QCOMPARE(tabs.viewSettings().zoomPercent, 110);
}

void ViewSettingsTest::setViewSettingsAppliesEverythingAtOnce()
{
    EditorTabs tabs;
    tabs.addEditor();

    ViewSettings settings;
    settings.wordWrap = false;
    settings.zoomPercent = 150;
    settings.font = QFont(QStringLiteral("Courier New"));
    settings.font.setPointSizeF(10.0);
    tabs.setViewSettings(settings);

    QCOMPARE(tabs.viewSettings(), settings);
    for (int i = 0; i < tabs.count(); ++i) {
        QCOMPARE(tabs.editorAt(i)->lineWrapMode(), QPlainTextEdit::NoWrap);
        QCOMPARE(tabs.editorAt(i)->font().pointSizeF(), 15.0);
    }
}

void ViewSettingsTest::setViewSettingsClampsTheZoom()
{
    EditorTabs tabs;
    ViewSettings settings;

    settings.zoomPercent = 9000;
    tabs.setViewSettings(settings);
    QCOMPARE(tabs.viewSettings().zoomPercent, 500);

    settings.zoomPercent = -4;
    tabs.setViewSettings(settings);
    QCOMPARE(tabs.viewSettings().zoomPercent, 10);
}

void ViewSettingsTest::setViewSettingsEmitsOnceAndOnlyForRealChanges()
{
    EditorTabs tabs;
    QSignalSpy spy(&tabs, &EditorTabs::viewSettingsChanged);

    ViewSettings settings;
    settings.wordWrap = false;
    settings.zoomPercent = 130;
    settings.font.setPointSizeF(14.0); // three things differ ...
    tabs.setViewSettings(settings);
    QCOMPARE(spy.count(), 1); // ... but listeners hear about it once

    tabs.setViewSettings(settings); // same again
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(ViewSettingsTest)

#include "ViewSettingsTest.moc"
