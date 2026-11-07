#include "EncodingCommands.h"

#include "DocumentController.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

using scribe::core::Encoding;
using scribe::core::EncodingInfo;
using scribe::core::LineEnding;

const QList<EncodingCommands::EncodingEntry>& EncodingCommands::encodings()
{
    static const QList<EncodingEntry> entries = {
        {QLatin1String("encoding.utf8"), {Encoding::Utf8, false}},
        {QLatin1String("encoding.utf8Bom"), {Encoding::Utf8, true}},
        {QLatin1String("encoding.utf16Le"), {Encoding::Utf16Le, true}},
        {QLatin1String("encoding.utf16LeNoBom"), {Encoding::Utf16Le, false}},
        {QLatin1String("encoding.utf16Be"), {Encoding::Utf16Be, true}},
        {QLatin1String("encoding.utf16BeNoBom"), {Encoding::Utf16Be, false}},
        {QLatin1String("encoding.ansi"), {Encoding::Ansi, false}},
    };
    return entries;
}

const QList<EncodingCommands::LineEndingEntry>& EncodingCommands::lineEndings()
{
    static const QList<LineEndingEntry> entries = {
        {QLatin1String("lineEnding.crlf"), LineEnding::Crlf},
        {QLatin1String("lineEnding.lf"), LineEnding::Lf},
        {QLatin1String("lineEnding.cr"), LineEnding::Cr},
    };
    return entries;
}

QString EncodingCommands::lineEndingTitle(LineEnding lineEnding)
{
    switch (lineEnding) {
        case LineEnding::Crlf:
            return tr("&Windows (CRLF)");
        case LineEnding::Lf:
            return tr("&Unix (LF)");
        case LineEnding::Cr:
            return tr("&Old Mac (CR)");
    }
    return {};
}

void EncodingCommands::registerIn(scribe::core::CommandRegistry& registry,
                                  DocumentController& controller, scribe::editor::EditorTabs& tabs)
{
    using scribe::core::Command;

    for (const EncodingEntry& entry : encodings()) {
        registry.registerCommand(Command{
            .id = entry.id,
            .title = scribe::core::encodingName(entry.encoding),
            .shortcut = {},
            .callback = [&controller,
                         encoding = entry.encoding] { controller.setEncoding(encoding); },
            .enabled = true,
            .enabledWhen = {},
            .checkedWhen =
                [&tabs, encoding = entry.encoding] {
                    return tabs.currentEditor()->fileDocument().encoding() == encoding;
                },
            .alternateShortcut = {},
        });
    }

    for (const LineEndingEntry& entry : lineEndings()) {
        registry.registerCommand(Command{
            .id = entry.id,
            .title = lineEndingTitle(entry.lineEnding),
            .shortcut = {},
            .callback = [&controller,
                         ending = entry.lineEnding] { controller.setLineEnding(ending); },
            .enabled = true,
            .enabledWhen = {},
            .checkedWhen =
                [&tabs, ending = entry.lineEnding] {
                    return tabs.currentEditor()->fileDocument().lineEnding() == ending;
                },
            .alternateShortcut = {},
        });
    }
}
