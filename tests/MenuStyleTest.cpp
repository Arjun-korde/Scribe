#include "MenuStyle.h"

#include "Theme.h"
#include "ThemeManager.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QStyleHints>
#include <QStyleOptionMenuItem>
#include <QtTest>

using scribe::app::MenuStyle;
using scribe::app::Theme;
using scribe::app::ThemeManager;

/// The menus follow the Edit menu in the Figma file (frame "Editor Context Menu").
class MenuStyleTest : public QObject
{
    Q_OBJECT

private:
    // The style the application got from the ThemeManager, with its palette.
    static QStyle* appStyle() { return QApplication::style(); }

    static QMenu* editLikeMenu(QWidget* parent)
    {
        auto* menu = new QMenu(parent);
        menu->addAction("Cut")->setEnabled(false);
        menu->addAction("Copy");
        menu->addAction("Paste");
        menu->addSeparator();
        menu->addAction("Select All");
        menu->addSeparator();
        menu->addAction("Start dictation");
        return menu;
    }

private slots:
    void initTestCase()
    {
        // Lives for the whole test run, like in the application.
        static ThemeManager manager(*qobject_cast<QApplication*>(QApplication::instance()));
        Q_UNUSED(manager);
    }

    void rowsAreTwentyEightPixelsWithTwoPixelsBetweenThem()
    {
        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        item.text = "Copy\tCtrl+C";
        item.font = QApplication::font("QMenu");
        const QSize size = appStyle()->sizeFromContents(QStyle::CT_MenuItem, &item, QSize());
        QCOMPARE(size.height(), MenuStyle::kRowHeight + MenuStyle::kRowGap);
    }

    void separatorIsOnePixelWithAPixelAroundIt()
    {
        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Separator;
        const QSize size = appStyle()->sizeFromContents(QStyle::CT_MenuItem, &item, QSize());
        QCOMPARE(size.height(), MenuStyle::kSeparatorRow);
    }

    void rowsAreAtLeastTheDesignWidth()
    {
        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        item.text = "Cut\tCtrl+X";
        item.font = QApplication::font("QMenu");
        QWidget parent;
        auto* menu = new QMenu(&parent);
        menu->addAction("Cut");
        menu->resize(menu->sizeHint());
        QCOMPARE(menu->actionGeometry(menu->actions().first()).width(), MenuStyle::kRowWidth);
        QCOMPARE(MenuStyle::kRowWidth, 250);
    }

    void rowsWithShortcutsAreNotWiderThanRowsWithout()
    {
        // QMenu adds the width of its widest shortcut to every row; the style takes it off again,
        // so a menu full of shortcuts has the same 250 px rows as one without any.
        QWidget parent;
        auto* menu = new QMenu(&parent);
        menu->addAction("Open")->setShortcut(QKeySequence("Ctrl+O"));
        menu->addAction("Save As")->setShortcut(QKeySequence("Ctrl+Shift+S"));
        menu->addSeparator();
        menu->addAction("Exit");
        menu->resize(menu->sizeHint());

        for (QAction* action : menu->actions()) {
            QCOMPARE(menu->actionGeometry(action).width(), MenuStyle::kRowWidth);
        }
    }

    void aLongLabelWidensEveryRowOfTheMenuEqually()
    {
        QWidget parent;
        auto* menu = new QMenu(&parent);
        menu->addAction(QString("x").repeated(60))->setShortcut(QKeySequence("Ctrl+Shift+S"));
        menu->addAction("Short")->setShortcut(QKeySequence("Ctrl+B"));
        menu->resize(menu->sizeHint());

        const int first = menu->actionGeometry(menu->actions().at(0)).width();
        QVERIFY(first > MenuStyle::kRowWidth);
        QCOMPARE(menu->actionGeometry(menu->actions().at(1)).width(), first);
    }

    void aLongLabelWidensTheRow()
    {
        QStyleOptionMenuItem item;
        item.menuItemType = QStyleOptionMenuItem::Normal;
        item.font = QApplication::font("QMenu");
        item.text = QString("x").repeated(80) + "\tCtrl+Shift+Alt+F12";
        QVERIFY(appStyle()->sizeFromContents(QStyle::CT_MenuItem, &item, QSize()).width() >
                MenuStyle::kRowWidth);
    }

    void figmaEditorContextMenuIs164PixelsHigh()
    {
        // The design's menu: Cut, Copy, Paste | Select All | Start dictation, 164 px high
        // (1 px border, 4 px padding, 28 px rows, 2 px between them). The width is ours: 250 px
        // rows.
        QWidget parent;
        QMenu* menu = editLikeMenu(&parent);
        QCOMPARE(menu->sizeHint().height(), 164);
    }

    void panelAndBorderUseTheThemeColours()
    {
        QWidget parent;
        QMenu* menu = editLikeMenu(&parent);
        menu->resize(menu->sizeHint());
        const Theme theme = ThemeManager::themeFor(QApplication::styleHints()->colorScheme());

        const QImage image = menu->grab().toImage();
        const qreal ratio = image.devicePixelRatio();
        // Inside the panel, between two rows: the surface colour.
        QCOMPARE(image.pixelColor(int(8 * ratio), 4).rgb(), theme.surface.rgb());
        // The outermost column is the outline.
        QCOMPARE(image.pixelColor(0, int(80 * ratio)).rgb(), theme.outline.rgb());
    }

    void hoveredRowGetsTheHoverFillAndDisabledRowIsFaded()
    {
        QWidget parent;
        QMenu* menu = editLikeMenu(&parent);
        menu->setActiveAction(menu->actions().at(2)); // Paste
        menu->resize(menu->sizeHint());
        const Theme theme = ThemeManager::themeFor(QApplication::styleHints()->colorScheme());
        const qreal ratio = menu->devicePixelRatioF();

        const QRect paste = menu->actionGeometry(menu->actions().at(2));
        const QRect cut = menu->actionGeometry(menu->actions().at(0));
        const QImage image = menu->grab().toImage();

        // Left padding of the row, away from the text.
        QCOMPARE(image.pixelColor(int((paste.left() + 8) * ratio), int(paste.center().y() * ratio))
                     .rgb(),
                 theme.hover.rgb());
        // The fill reaches the right edge of the row and stops there; beyond it is the padding.
        QCOMPARE(image.pixelColor(int((paste.right() - 1) * ratio), int(paste.center().y() * ratio))
                     .rgb(),
                 theme.hover.rgb());
        QCOMPARE(image.pixelColor(int((paste.right() + 2) * ratio), int(paste.center().y() * ratio))
                     .rgb(),
                 theme.surface.rgb());
        // The disabled row is not filled.
        QCOMPARE(
            image.pixelColor(int((cut.left() + 8) * ratio), int(cut.center().y() * ratio)).rgb(),
            theme.surface.rgb());
    }

    void labelAndShortcutAreDrawnOnOppositeSides()
    {
        QWidget parent;
        auto* menu = new QMenu(&parent);
        auto* copy = menu->addAction("Copy");
        copy->setShortcut(QKeySequence("Ctrl+C"));
        menu->resize(menu->sizeHint());
        const Theme theme = ThemeManager::themeFor(QApplication::styleHints()->colorScheme());
        const qreal ratio = menu->devicePixelRatioF();

        const QRect row = menu->actionGeometry(copy);
        const QImage image = menu->grab().toImage();
        const auto hasInk = [&](int from, int to) {
            for (int x = int(from * ratio); x < int(to * ratio); ++x) {
                for (int y = int(row.top() * ratio); y < int(row.bottom() * ratio); ++y) {
                    if (image.pixelColor(x, y).rgb() != theme.surface.rgb()) {
                        return true;
                    }
                }
            }
            return false;
        };
        const int middle = row.center().x();
        QVERIFY(hasInk(row.left(), row.left() + 60));   // the label, on the left
        QVERIFY(hasInk(row.right() - 60, row.right())); // the shortcut, on the right
        QVERIFY(!hasInk(row.left() + 70, middle));      // and nothing in between
    }

    void checkedItemsReserveAColumnAndShowAMark()
    {
        QWidget parent;
        auto* menu = new QMenu(&parent);
        auto* checked = menu->addAction("Word Wrap");
        checked->setCheckable(true);
        checked->setChecked(true);
        auto* plain = menu->addAction("Font...");
        menu->resize(menu->sizeHint());
        const Theme theme = ThemeManager::themeFor(QApplication::styleHints()->colorScheme());
        const qreal ratio = menu->devicePixelRatioF();

        const auto inkInColumn = [&](QAction* action) {
            const QRect row = menu->actionGeometry(action);
            const QImage image = menu->grab().toImage();
            for (int x = int((row.left() + MenuStyle::kPaddingX) * ratio);
                 x < int((row.left() + MenuStyle::kPaddingX + 12) * ratio); ++x) {
                for (int y = int(row.top() * ratio); y < int(row.bottom() * ratio); ++y) {
                    if (image.pixelColor(x, y).rgb() != theme.surface.rgb()) {
                        return true;
                    }
                }
            }
            return false;
        };
        QVERIFY(inkInColumn(checked)); // the check mark
        QVERIFY(!inkInColumn(plain));  // the column is kept, but empty
    }
};

QTEST_MAIN(MenuStyleTest)
#include "MenuStyleTest.moc"
