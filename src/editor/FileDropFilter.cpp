#include "FileDropFilter.h"

#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QTimer>
#include <QUrl>

namespace scribe::editor
{

FileDropFilter::FileDropFilter(QObject* parent)
    : QObject(parent)
{}

QStringList FileDropFilter::localFiles(const QMimeData& mime)
{
    QStringList files;
    for (const QUrl& url : mime.urls()) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isFile()) {
            files.append(path);
        }
    }
    return files;
}

bool FileDropFilter::eventFilter(QObject* watched, QEvent* event)
{
    const QEvent::Type type = event->type();
    if (type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop) {
        return QObject::eventFilter(watched, event);
    }

    // QDragEnterEvent and QDragMoveEvent derive from QDropEvent, which has all we need here.
    auto* drag = static_cast<QDropEvent*>(event);
    const QMimeData* mime = drag->mimeData();
    if (mime == nullptr || !mime->hasUrls()) {
        return QObject::eventFilter(watched, event);
    }

    const QStringList files = localFiles(*mime);
    if (files.isEmpty()) {
        // hasText() cannot be used here: for a plain list of URLs Qt reports true because it can
        // synthesize text from them. Only an explicit text/plain format means the drag really
        // carries text, as a link dragged from a browser does.
        if (mime->hasFormat(QStringLiteral("text/plain"))) {
            return QObject::eventFilter(watched, event); // a link: let the editor insert the text
        }
        drag->ignore(); // only folders or network URLs: refuse, and keep the editor out of it
        return true;
    }

    drag->setDropAction(Qt::CopyAction);
    drag->accept();
    if (type == QEvent::Drop) {
        QTimer::singleShot(0, this, [this, files] { emit filesDropped(files); });
    }
    return true;
}

} // namespace scribe::editor
