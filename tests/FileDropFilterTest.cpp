#include "FileDropFilter.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#include <memory>

using scribe::editor::EditorTabs;
using scribe::editor::FileDropFilter;

namespace
{

// Records which drag and drop handlers a widget's own code received, i.e. which events the
// filter did NOT consume.
class Target : public QWidget
{
public:
    Target() { setAcceptDrops(true); } // Qt only delivers drag events to widgets that ask for them

    bool enterReached = false;
    bool dropReached = false;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override
    {
        enterReached = true;
        event->acceptProposedAction();
    }
    void dropEvent(QDropEvent* event) override
    {
        dropReached = true;
        event->acceptProposedAction();
    }
};

} // namespace

class FileDropFilterTest : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void localFilesKeepsOnlyExistingLocalFilesInOrder();

    void dragWithFilesIsAcceptedAsACopyAndStaysAwayFromTheWidget();
    void dragMoveWithFilesIsAccepted();
    void dragOfOnlyAFolderIsRefusedAndStaysAwayFromTheWidget();
    void dragOfOnlyANetworkUrlIsRefused();
    void dragOfTextPassesThrough();
    void dragOfALinkThatCarriesTextPassesThrough();

    void dropReportsFilesInOrderAfterTheDropHasReturned();
    void dropIgnoresFoldersMixedWithFiles();
    void dropOfOnlyAFolderReportsNothing();
    void dropOfTextIsNotConsumed();

    void droppingOnAnEditorEmitsOnTheTabsAndInsertsNothing();
    void droppingOnTheTabStripEmitsOnTheTabs();
    void droppingTextOnAnEditorStillInsertsIt();
    void newTabsAcceptFileDropsToo();

private:
    QString makeFile(const QString& name) const;
    QString makeFolder(const QString& name) const;
    static QMimeData* urlsFor(const QStringList& paths);
    static bool sendEnter(QWidget& target, const QMimeData& mime, bool move = false);
    static bool sendDrop(QWidget& target, const QMimeData& mime);

    std::unique_ptr<QTemporaryDir> m_dir;
};

void FileDropFilterTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

QString FileDropFilterTest::makeFile(const QString& name) const
{
    QFile file(m_dir->filePath(name));
    file.open(QIODevice::WriteOnly);
    file.write(name.toUtf8());
    return file.fileName();
}

QString FileDropFilterTest::makeFolder(const QString& name) const
{
    QDir(m_dir->path()).mkdir(name);
    return m_dir->filePath(name);
}

QMimeData* FileDropFilterTest::urlsFor(const QStringList& paths)
{
    auto* mime = new QMimeData; // the caller owns it; tests wrap it in a unique_ptr
    QList<QUrl> urls;
    for (const QString& path : paths) {
        urls.append(path.startsWith(QStringLiteral("http")) ? QUrl(path)
                                                            : QUrl::fromLocalFile(path));
    }
    mime->setUrls(urls);
    return mime;
}

// Returns whether the event ended up accepted.
bool FileDropFilterTest::sendEnter(QWidget& target, const QMimeData& mime, bool move)
{
    QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    enter.setAccepted(false);
    QApplication::sendEvent(&target, &enter);
    if (!move || !enter.isAccepted()) {
        return enter.isAccepted();
    }

    // Moving over the widget comes after the enter, as in a real drag.
    QDragMoveEvent moveEvent(QPoint(6, 6), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    moveEvent.setAccepted(false);
    QApplication::sendEvent(&target, &moveEvent);
    return moveEvent.isAccepted();
}

bool FileDropFilterTest::sendDrop(QWidget& target, const QMimeData& mime)
{
    // A real drag always enters a widget before it drops, and Qt routes the drop to the widget
    // that accepted the enter. If the enter is refused the operating system sends no drop at all.
    if (!sendEnter(target, mime)) {
        return false;
    }
    QDropEvent event(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    event.setAccepted(false);
    QApplication::sendEvent(&target, &event);
    return event.isAccepted();
}

void FileDropFilterTest::localFilesKeepsOnlyExistingLocalFilesInOrder()
{
    const QString b = makeFile("b.txt");
    const QString a = makeFile("a.txt");
    const QString folder = makeFolder("dir");
    const std::unique_ptr<QMimeData> mime(
        urlsFor({b, folder, m_dir->filePath("missing.txt"), "http://example.com/x.txt", a}));

    QCOMPARE(FileDropFilter::localFiles(*mime), (QStringList{b, a}));
}

void FileDropFilterTest::dragWithFilesIsAcceptedAsACopyAndStaysAwayFromTheWidget()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    const std::unique_ptr<QMimeData> mime(urlsFor({makeFile("a.txt")}));

    QDragEnterEvent event(QPoint(5, 5), Qt::MoveAction | Qt::CopyAction, mime.get(), Qt::LeftButton,
                          Qt::NoModifier);
    event.setAccepted(false);
    QApplication::sendEvent(&target, &event);

    QVERIFY(event.isAccepted());
    QCOMPARE(event.dropAction(), Qt::CopyAction); // the cursor shows "copy", never "move"
    QVERIFY(!target.enterReached);
}

void FileDropFilterTest::dragMoveWithFilesIsAccepted()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    const std::unique_ptr<QMimeData> mime(urlsFor({makeFile("a.txt")}));

    QVERIFY(sendEnter(target, *mime, /*move=*/true));
}

void FileDropFilterTest::dragOfOnlyAFolderIsRefusedAndStaysAwayFromTheWidget()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    const std::unique_ptr<QMimeData> mime(urlsFor({makeFolder("dir")}));

    QVERIFY(!sendEnter(target, *mime));
    QVERIFY(!target.enterReached); // swallowed, so a text editor cannot insert the path
}

void FileDropFilterTest::dragOfOnlyANetworkUrlIsRefused()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    const std::unique_ptr<QMimeData> mime(urlsFor({"http://example.com/page"}));

    QVERIFY(!sendEnter(target, *mime));
    QVERIFY(!target.enterReached);
}

void FileDropFilterTest::dragOfTextPassesThrough()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    QMimeData mime;
    mime.setText(QStringLiteral("some text"));

    QVERIFY(sendEnter(target, mime)); // accepted by the widget's own handler
    QVERIFY(target.enterReached);
}

void FileDropFilterTest::dragOfALinkThatCarriesTextPassesThrough()
{
    // Dragging a link out of a browser carries a URL and its text: the editor may insert the text.
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    QMimeData mime;
    mime.setUrls({QUrl(QStringLiteral("http://example.com/page"))});
    mime.setText(QStringLiteral("http://example.com/page"));

    QVERIFY(sendEnter(target, mime));
    QVERIFY(target.enterReached);
}

void FileDropFilterTest::dropReportsFilesInOrderAfterTheDropHasReturned()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    QSignalSpy spy(&filter, &FileDropFilter::filesDropped);
    const QString first = makeFile("one.txt");
    const QString second = makeFile("two.txt");
    const std::unique_ptr<QMimeData> mime(urlsFor({first, second}));

    QVERIFY(sendDrop(target, *mime));
    QCOMPARE(spy.count(), 0); // not from inside the drop: Explorer is still waiting for it
    QVERIFY(!target.dropReached);

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toStringList(), (QStringList{first, second}));
}

void FileDropFilterTest::dropIgnoresFoldersMixedWithFiles()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    QSignalSpy spy(&filter, &FileDropFilter::filesDropped);
    const QString file = makeFile("a.txt");
    const std::unique_ptr<QMimeData> mime(urlsFor({makeFolder("dir"), file}));

    QVERIFY(sendDrop(target, *mime));
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.first().first().toStringList(), (QStringList{file}));
}

void FileDropFilterTest::dropOfOnlyAFolderReportsNothing()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    QSignalSpy spy(&filter, &FileDropFilter::filesDropped);
    const std::unique_ptr<QMimeData> mime(urlsFor({makeFolder("dir")}));

    QVERIFY(!sendDrop(target, *mime));
    QVERIFY(!spy.wait(100));
    QVERIFY(!target.dropReached);
}

void FileDropFilterTest::dropOfTextIsNotConsumed()
{
    Target target;
    FileDropFilter filter;
    target.installEventFilter(&filter);
    QSignalSpy spy(&filter, &FileDropFilter::filesDropped);
    QMimeData mime;
    mime.setText(QStringLiteral("text"));

    QVERIFY(sendDrop(target, mime));
    QVERIFY(target.dropReached);
    QVERIFY(!spy.wait(100));
}

void FileDropFilterTest::droppingOnAnEditorEmitsOnTheTabsAndInsertsNothing()
{
    EditorTabs tabs;
    QSignalSpy spy(&tabs, &EditorTabs::filesDropped);
    const QString file = makeFile("a.txt");
    const std::unique_ptr<QMimeData> mime(urlsFor({file}));

    QVERIFY(sendDrop(*tabs.currentEditor()->viewport(), *mime));
    QVERIFY(spy.wait(1000));

    QCOMPARE(spy.first().first().toStringList(), (QStringList{file}));
    QCOMPARE(tabs.currentEditor()->toPlainText(), QString()); // the path was not typed in
    QVERIFY(!tabs.currentEditor()->isModified());
}

void FileDropFilterTest::droppingOnTheTabStripEmitsOnTheTabs()
{
    EditorTabs tabs;
    tabs.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabs));
    QSignalSpy spy(&tabs, &EditorTabs::filesDropped);
    const QString file = makeFile("a.txt");
    const std::unique_ptr<QMimeData> mime(urlsFor({file}));

    QVERIFY(sendDrop(tabs, *mime));
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.first().first().toStringList(), (QStringList{file}));
}

void FileDropFilterTest::droppingTextOnAnEditorStillInsertsIt()
{
    EditorTabs tabs;
    QMimeData mime;
    mime.setText(QStringLiteral("dropped text"));

    QVERIFY(sendDrop(*tabs.currentEditor()->viewport(), mime));
    QCOMPARE(tabs.currentEditor()->toPlainText(), QStringLiteral("dropped text"));
}

void FileDropFilterTest::newTabsAcceptFileDropsToo()
{
    EditorTabs tabs;
    tabs.addEditor();
    tabs.addEditor();
    QSignalSpy spy(&tabs, &EditorTabs::filesDropped);
    const std::unique_ptr<QMimeData> mime(urlsFor({makeFile("a.txt")}));

    QVERIFY(sendDrop(*tabs.editorAt(2)->viewport(), *mime));
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(FileDropFilterTest)

#include "FileDropFilterTest.moc"
