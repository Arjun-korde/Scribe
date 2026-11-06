#include "FileDialogs.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

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
    QMessageBox box(QMessageBox::Warning, tr("Scribe"),
                    tr("Do you want to save changes to \"%1\"?").arg(documentName),
                    QMessageBox::NoButton, parent);
    box.setInformativeText(tr("Your changes will be lost if you don't save them."));

    QPushButton* save = box.addButton(tr("&Save"), QMessageBox::AcceptRole);
    QPushButton* discard = box.addButton(tr("Do&n't Save"), QMessageBox::DestructiveRole);
    QPushButton* cancel = box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    box.setDefaultButton(save);
    box.setEscapeButton(cancel); // Esc and the window's close button mean Cancel

    box.exec();
    if (box.clickedButton() == save) {
        return UnsavedChoice::Save;
    }
    if (box.clickedButton() == discard) {
        return UnsavedChoice::Discard;
    }
    return UnsavedChoice::Cancel;
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
    return dialogs;
}
