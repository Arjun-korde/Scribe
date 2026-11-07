#include "GoToLineDialog.h"

#include "LineNumber.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

GoToLineDialog::GoToLineDialog(int currentLine, int lineCount, QWidget* parent)
    : QDialog(parent)
    , m_lineCount(lineCount)
{
    setWindowTitle(tr("Go To Line"));

    auto* prompt = new QLabel(tr("&Line number (1 to %1):").arg(lineCount), this);

    m_field = new QLineEdit(this);
    m_field->setAccessibleName(tr("Line number"));
    // Digits only, so letters and signs cannot be typed. The range is not enforced here: typing a
    // number that is too big must be possible, so that the hint can explain what is wrong.
    m_field->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[0-9]{0,10}")), m_field));
    m_field->setText(QString::number(currentLine));
    m_field->selectAll();
    prompt->setBuddy(m_field);

    m_hint = new QLabel(this);
    m_hint->setWordWrap(true);
    m_hint->setMinimumHeight(m_hint->fontMetrics().height()); // no jump when the hint appears

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_goButton = buttons->button(QDialogButtonBox::Ok);
    m_goButton->setText(tr("Go"));
    m_goButton->setDefault(true);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(prompt);
    layout->addWidget(m_field);
    layout->addWidget(m_hint);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_field, &QLineEdit::textChanged, this, &GoToLineDialog::updateState);

    updateState();
    m_field->setFocus();
}

std::optional<int> GoToLineDialog::line() const
{
    return scribe::core::parseLineNumber(m_field->text(), m_lineCount);
}

QString GoToLineDialog::hintText(int lineCount)
{
    return tr("Enter a number from 1 to %1").arg(lineCount);
}

void GoToLineDialog::updateState()
{
    const bool valid = line().has_value();

    // A disabled default button is why Enter does nothing while the number is invalid.
    m_goButton->setEnabled(valid);

    const QString hint = valid ? QString() : hintText(m_lineCount);
    m_hint->setText(hint);
    m_field->setAccessibleDescription(hint); // read out by screen readers, not only seen
    m_field->setProperty("invalid", !valid);
}
