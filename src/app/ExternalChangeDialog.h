#pragma once

#include "CardDialog.h"
#include "FileDialogs.h"

class QLabel;
class QPushButton;

namespace scribe::app
{

/// "notes.txt changed on disk" with Reload and Keep current, or "notes.txt was deleted" with Keep
/// and Close tab. The safe choice (Keep) is never the one Enter or Esc picks for a change that
/// would lose edits: with unsaved changes the default is Keep current. Esc means Keep.
class ExternalChangeDialog : public CardDialog
{
    Q_OBJECT

public:
    ExternalChangeDialog(const QString& documentName, ExternalChangeKind kind,
                         bool hasUnsavedChanges, QWidget* parent = nullptr);

    /// What the user chose. Keep until a button says otherwise (and after Esc).
    ExternalChangeChoice choice() const { return m_choice; }

    QLabel* titleLabel() const { return m_title; }
    QLabel* bodyLabel() const { return m_body; }
    /// Reload, or Close tab for a deleted file.
    QPushButton* primaryButton() const { return m_primary; }
    QPushButton* keepButton() const { return m_keep; }

    static constexpr int kCardWidth = 440;

private:
    QString titleFor(const QString& documentName, ExternalChangeKind kind) const;

    QLabel* m_title = nullptr;
    QLabel* m_body = nullptr;
    QPushButton* m_primary = nullptr;
    QPushButton* m_keep = nullptr;
    ExternalChangeChoice m_choice = ExternalChangeChoice::Keep;
};

} // namespace scribe::app
