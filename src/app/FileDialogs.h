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
