#include "FileDialogs.h"

#include "GoToLineDialog.h"
#include "UnsavedChangesDialog.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>

namespace
{

QString tr(const char* text)
{
    return QCoreApplication::translate("FileDialogs", text);
}

QString fileFilter()
{
    return tr("Text Documents (*.txt);;All Files (*)");
}

UnsavedChoice askAboutUnsavedChanges(QWidget* parent, const QString& documentName)
{
    scribe::app::UnsavedChangesDialog dialog(documentName, parent);
    dialog.exec();
    return dialog.choice(); // Cancel unless a button said otherwise, so Esc never discards
}

} // namespace

FileDialogs defaultFileDialogs()
{
    FileDialogs dialogs;
    dialogs.chooseOpenPath = [](QWidget* parent) {
        return QFileDialog::getOpenFileName(parent, tr("Open"), QString(), fileFilter());
    };
    dialogs.chooseSavePath = [](QWidget* parent, const QString& suggestedPath) {
        return QFileDialog::getSaveFileName(parent, tr("Save As"), suggestedPath, fileFilter());
    };
    dialogs.showError = [](QWidget* parent, const QString& title, const QString& message) {
        QMessageBox::warning(parent, title, message);
    };
    dialogs.confirmDiscard = askAboutUnsavedChanges;
    dialogs.chooseFont = [](QWidget* parent, const QFont& current) -> std::optional<QFont> {
        bool accepted = false;
        const QFont font = QFontDialog::getFont(&accepted, current, parent, tr("Font"));
        return accepted ? std::optional<QFont>(font) : std::nullopt;
    };
    dialogs.askLineNumber = [](QWidget* parent, int currentLine,
                               int lineCount) -> std::optional<int> {
        GoToLineDialog dialog(currentLine, lineCount, parent);
        return dialog.exec() == QDialog::Accepted ? dialog.line() : std::nullopt;
    };
    return dialogs;
}
