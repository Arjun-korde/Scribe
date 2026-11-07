#pragma once

#include "CommandRegistry.h"
#include "Encoding.h"
#include "LineEnding.h"

#include <QCoreApplication>
#include <QLatin1String>
#include <QList>

class DocumentController;

namespace scribe::editor
{
class EditorTabs;
} // namespace scribe::editor

/// The Encoding menu commands: choose how the active document is written when it is saved.
/// One command per encoding and one per line ending, each a radio-style toggle whose check mark
/// shows the active document's current choice.
///
/// These change how the next save writes the file; they do not re-read it. Opening a file with
/// another encoding is a different feature ("Reopen with encoding").
class EncodingCommands
{
    Q_DECLARE_TR_FUNCTIONS(EncodingCommands)

public:
    struct EncodingEntry
    {
        QLatin1String id;
        scribe::core::EncodingInfo encoding;
    };

    struct LineEndingEntry
    {
        QLatin1String id;
        scribe::core::LineEnding lineEnding;
    };

    /// Every encoding the editor can write, in menu order. The menu text is
    /// scribe::core::encodingName() of the entry.
    static const QList<EncodingEntry>& encodings();

    /// Windows (CRLF), Unix (LF) and Old Mac (CR), in menu order.
    static const QList<LineEndingEntry>& lineEndings();

    /// The menu text of a line ending command, such as "Windows (CRLF)".
    static QString lineEndingTitle(scribe::core::LineEnding lineEnding);

    /// Adds the commands to `registry`. They keep references to `controller` and `tabs`, which must
    /// outlive every call to registry.run() or registry.isChecked().
    static void registerIn(scribe::core::CommandRegistry& registry, DocumentController& controller,
                           scribe::editor::EditorTabs& tabs);
};
