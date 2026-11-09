#pragma once

#include "CardDialog.h"
#include "FileDialogs.h"

class QLabel;
class QPushButton;

namespace scribe::app
{

/// "Save changes to todo.md?" with Save, Don't save and Cancel, as in the design. Save is the
/// default button, so Enter saves. Esc and Alt+F4 mean Cancel, never a silent discard.
class UnsavedChangesDialog : public CardDialog
{
    Q_OBJECT

public:
    UnsavedChangesDialog(const QString& documentName, QWidget* parent = nullptr);

    /// What the user chose. Cancel until a button says otherwise (and after Esc).
    UnsavedChoice choice() const { return m_choice; }

    // The parts, for tests and for whoever needs to describe the dialog to the user.
    QLabel* titleLabel() const { return m_title; }
    QLabel* bodyLabel() const { return m_body; }
    QPushButton* saveButton() const { return m_save; }
    QPushButton* discardButton() const { return m_discard; }
    QPushButton* cancelButton() const { return m_cancel; }

    /// The card is this wide in the design (the window is wider by the shadow margin).
    static constexpr int kCardWidth = 440;

private:
    /// The title for `documentName`, with the name shortened in the middle if it is too long to
    /// fit the card on one line.
    QString titleFor(const QString& documentName) const;

    QLabel* m_title = nullptr;
    QLabel* m_body = nullptr;
    QPushButton* m_save = nullptr;
    QPushButton* m_discard = nullptr;
    QPushButton* m_cancel = nullptr;
    UnsavedChoice m_choice = UnsavedChoice::Cancel;
};

} // namespace scribe::app
