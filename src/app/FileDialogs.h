#pragma once

#include <QFont>
#include <QString>

#include <cstdint>
#include <functional>
#include <optional>

class QWidget;

/// What the user chose when asked about unsaved changes.
enum class UnsavedChoice : std::uint8_t
{
    Save,    ///< Write the changes, then continue.
    Discard, ///< Throw the changes away and continue.
    Cancel,  ///< Stop; keep everything as it is.
};

/// What happened to an open file behind the editor's back.
enum class ExternalChangeKind : std::uint8_t
{
    Changed, ///< Another program wrote to the file.
    Deleted, ///< The file was deleted or moved away.
};

/// What the user chose when told about such a change.
enum class ExternalChangeChoice : std::uint8_t
{
    Reload, ///< Read the file again (only offered for Changed).
    Keep,   ///< Keep the text in the editor as it is.
    Close,  ///< Close the tab (only offered for Deleted).
};

/// The user interaction the application needs from the outside world. The defaults show the
/// standard Qt dialogs; tests replace them with scripted answers so no dialog ever blocks.
struct FileDialogs
{
    /// Asks which file to open. An empty result means the user cancelled.
    std::function<QString(QWidget* parent)> chooseOpenPath;

    /// Asks where to save. `suggestedPath` is the current file, if any. Empty means cancelled.
    std::function<QString(QWidget* parent, const QString& suggestedPath)> chooseSavePath;

    /// Tells the user that something failed.
    std::function<void(QWidget* parent, const QString& title, const QString& message)> showError;

    /// Asks what to do with the unsaved changes of the document called `documentName`. A missing
    /// function counts as Cancel, so changes are never discarded by accident.
    std::function<UnsavedChoice(QWidget* parent, const QString& documentName)> confirmDiscard;

    /// Tells the user that the file of the document called `documentName` changed on disk and asks
    /// what to do. `hasUnsavedChanges` says whether reloading would throw away edits. Reload is
    /// only a valid answer for Changed and Close only for Deleted. A missing function counts as
    /// Keep, so nothing is ever replaced or closed by accident.
    std::function<ExternalChangeChoice(QWidget* parent, const QString& documentName,
                                       ExternalChangeKind kind, bool hasUnsavedChanges)>
        confirmExternalChange;

    /// Asks for a font, starting from `current`. std::nullopt means the user cancelled. A missing
    /// function makes the Font command unavailable.
    std::function<std::optional<QFont>(QWidget* parent, const QFont& current)> chooseFont;

    /// Asks which line to go to: the line the caret is on and the number of lines in the document
    /// are given. std::nullopt means the user cancelled. A missing function makes the Go To Line
    /// command unavailable.
    std::function<std::optional<int>(QWidget* parent, int currentLine, int lineCount)>
        askLineNumber;
};

FileDialogs defaultFileDialogs();
