#include "UnsavedChangesDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace scribe::app
{

namespace
{
// Offsets of the design, in px: 1 px border + 24 px padding; 16 px between the text blocks and
// before the buttons; 8 px between the buttons.
constexpr int kCardPadding = 25;
constexpr int kBlockSpacing = 16;
constexpr int kButtonSpacing = 8;
} // namespace

UnsavedChangesDialog::UnsavedChangesDialog(const QString& documentName, QWidget* parent)
    : CardDialog(parent)
{
    setWindowTitle(tr("Scribe")); // not shown (no title bar), but used by the taskbar and Alt+Tab
    setModal(true);

    card()->setFixedWidth(kCardWidth);
    cardLayout()->setContentsMargins(kCardPadding, kCardPadding, kCardPadding, kCardPadding);

    m_title = new QLabel(card());
    m_title->setObjectName(QStringLiteral("dialogTitle")); // styled by the theme
    m_title->setText(titleFor(documentName));
    m_title->setAccessibleName(m_title->text());

    m_body = new QLabel(tr("Your changes will be lost if you don't save them."), card());
    m_body->setObjectName(QStringLiteral("dialogBody"));
    m_body->setWordWrap(true);

    m_save = new QPushButton(tr("&Save"), card());
    m_discard = new QPushButton(tr("Do&n't save"), card());
    m_cancel = new QPushButton(tr("Cancel"), card());
    m_save->setDefault(true); // the filled button, and what Enter presses

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(kButtonSpacing);
    buttons->addWidget(m_save);
    buttons->addWidget(m_discard);
    buttons->addWidget(m_cancel);
    buttons->addStretch();

    cardLayout()->addWidget(m_title);
    cardLayout()->addSpacing(kBlockSpacing);
    cardLayout()->addWidget(m_body);
    cardLayout()->addSpacing(kBlockSpacing);
    cardLayout()->addLayout(buttons);

    connect(m_save, &QPushButton::clicked, this, [this] {
        m_choice = UnsavedChoice::Save;
        accept();
    });
    connect(m_discard, &QPushButton::clicked, this, [this] {
        m_choice = UnsavedChoice::Discard;
        accept();
    });
    connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);

    m_save->setFocus();
}

QString UnsavedChangesDialog::titleFor(const QString& documentName) const
{
    // The label is styled by the theme (16 px, semibold), so measure with that font.
    m_title->ensurePolished();
    const QFontMetrics metrics(m_title->font());

    const QString whole = tr("Save changes to %1?");
    const int room =
        kCardWidth - 2 * kCardPadding - metrics.horizontalAdvance(whole.arg(QString()));
    return whole.arg(metrics.elidedText(documentName, Qt::ElideMiddle, std::max(room, 0)));
}

} // namespace scribe::app
