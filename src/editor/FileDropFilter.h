#pragma once

#include <QObject>
#include <QStringList>

class QMimeData;

namespace scribe::editor
{

/// Turns files dragged from Explorer into a `filesDropped` signal. Install it on every widget a
/// drop can land on (the text areas and the tab strip).
///
/// - A drag that contains local files is accepted with the "copy" cursor, and the files are
///   reported when it is dropped. Folders and non-local URLs in the same drag are ignored.
/// - A drag that has URLs but no plain-text format and no local files (a folder alone, or a network
///   URL alone) is refused. It is swallowed here so the text editor does not insert the path as
///   text.
/// - Everything else (text, or a link that also carries text) passes through untouched, so
///   dragging and dropping text keeps working.
class FileDropFilter : public QObject
{
    Q_OBJECT

public:
    explicit FileDropFilter(QObject* parent = nullptr);

    /// The local, existing, regular files in `mime`, in the order they were dragged. Folders,
    /// missing files and non-local URLs are left out.
    static QStringList localFiles(const QMimeData& mime);

    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    /// Emitted after a drop, never from inside it: Windows keeps the dragging program waiting
    /// until the drop returns, so opening files (and any error dialog) must happen afterwards.
    void filesDropped(const QStringList& paths);
};

} // namespace scribe::editor
