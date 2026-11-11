#include "ExternalChangeDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace scribe::app
{

namespace
{
// The same offsets as the unsaved-changes dialog: 1 px border + 24 px padding, 16 px between the
// blocks, 8 px between the buttons.
constexpr int kCardPadding = 25;
constexpr int kBlockSpacing = 16;
constexpr int kButtonSpacing = 8;
} // namespace

ExternalChangeDialog::ExternalChangeDialog(const QString& documentName, ExternalChangeKind kind,
                                           bool hasUnsavedChanges, QWidget* parent)
    : CardDialog(parent)
{
    setWindowTitle(tr("Scribe")); // not shown (no title bar), but used by the taskbar and Alt+Tab
    setModal(true);

    card()->setFixedWidth(kCardWidth);
    cardLayout()->setContentsMargins(kCardPadding, kCardPadding, kCardPadding, kCardPadding);

    m_title = new QLabel(card());
    m_title->setObjectName(QStringLiteral("dialogTitle")); // styled by the theme
    m_title->setText(titleFor(documentName, kind));
    m_title->setAccessibleName(m_title->text());

    QString body;
    if (kind == ExternalChangeKind::Deleted) {
        body = tr("Keep it open to save it again, or close the tab.");
    } else if (hasUnsavedChanges) {
        body = tr("You have unsaved changes here. Reloading will discard them.");
    } else {
        body = tr("Reload it to see the new version?");
    }
    m_body = new QLabel(body, card());
    m_body->setObjectName(QStringLiteral("dialogBody"));
    m_body->setWordWrap(true);

    const bool deleted = kind == ExternalChangeKind::Deleted;
    m_primary = new QPushButton(deleted ? tr("&Close tab") : tr("&Reload"), card());
    m_keep = new QPushButton(deleted ? tr("&Keep") : tr("&Keep current"), card());

    // Enter picks the safe answer when the other one would lose work.
    QPushButton* const defaultButton = (hasUnsavedChanges || deleted) ? m_keep : m_primary;
    defaultButton->setDefault(true);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(kButtonSpacing);
    buttons->addWidget(m_primary);
    buttons->addWidget(m_keep);
    buttons->addStretch();

    cardLayout()->addWidget(m_title);
    cardLayout()->addSpacing(kBlockSpacing);
    cardLayout()->addWidget(m_body);
    cardLayout()->addSpacing(kBlockSpacing);
    cardLayout()->addLayout(buttons);

    connect(m_primary, &QPushButton::clicked, this, [this, deleted] {
        m_choice = deleted ? ExternalChangeChoice::Close : ExternalChangeChoice::Reload;
        accept();
    });
    connect(m_keep, &QPushButton::clicked, this, [this] {
        m_choice = ExternalChangeChoice::Keep;
        accept();
    });

    defaultButton->setFocus();
}

QString ExternalChangeDialog::titleFor(const QString& documentName, ExternalChangeKind kind) const
{
    // The label is styled by the theme (16 px, semibold), so measure with that font.
    m_title->ensurePolished();
    const QFontMetrics metrics(m_title->font());

    const QString whole =
        kind == ExternalChangeKind::Deleted ? tr("%1 was deleted") : tr("%1 changed on disk");
    const int room =
        kCardWidth - 2 * kCardPadding - metrics.horizontalAdvance(whole.arg(QString()));
    return whole.arg(metrics.elidedText(documentName, Qt::ElideMiddle, std::max(room, 0)));
}

} // namespace scribe::app
