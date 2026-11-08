#pragma once

#include <QDialog>

#include <optional>

class QLabel;
class QLineEdit;
class QPushButton;

/// A small modal with one field for a line number. It opens on the current line, selected, so
/// typing replaces it. Enter confirms and Esc cancels.
///
/// Only a whole number from 1 to the number of lines is accepted. For anything else (for example
/// 310 in a 300-line document) the Go button is disabled, so Enter does nothing, and a hint says
/// what is allowed. The text hint is there so the problem is not shown by color alone; the field
/// is also marked with the dynamic property `invalid` (true/false) for the theme to style later.
class GoToLineDialog : public QDialog
{
    Q_OBJECT

public:
    GoToLineDialog(int currentLine, int lineCount, QWidget* parent = nullptr);

    /// The line typed so far, or std::nullopt while the text is not a valid line.
    [[nodiscard]] std::optional<int> line() const;

    /// "Enter a number from 1 to <lineCount>".
    static QString hintText(int lineCount);

private:
    void updateState();

    int m_lineCount;
    QLineEdit* m_field = nullptr;
    QLabel* m_hint = nullptr;
    QPushButton* m_goButton = nullptr;
};
