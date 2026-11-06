#include "EditorStatusBar.h"

#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QLabel>
#include <QTextBlock>
#include <QTextCursor>

namespace scribe::editor
{

EditorStatusBar::EditorStatusBar(EditorTabs& tabs, QWidget* parent)
    : QStatusBar(parent)
    , m_tabs(tabs)
{
    // The size grip is a Windows 7 era corner widget; the segments are what matters here.
    setSizeGripEnabled(false);

    m_position = addSegment(tr("Cursor position"));
    m_characters = addSegment(tr("Character count"));
    m_encoding = addSegment(tr("Encoding"));
    m_lineEnding = addSegment(tr("Line ending"));
    m_zoom = addSegment(tr("Zoom"));

    connect(&m_tabs, &EditorTabs::currentEditorStateChanged, this,
            &EditorStatusBar::followCurrentEditor);
    connect(&m_tabs, &EditorTabs::viewSettingsChanged, this, &EditorStatusBar::refresh);
    followCurrentEditor();
}

QString EditorStatusBar::positionText(int line, int column)
{
    return tr("Ln %1, Col %2").arg(line).arg(column);
}

QString EditorStatusBar::charactersText(qsizetype count, const QLocale& locale)
{
    return count == 1 ? tr("1 char") : tr("%1 chars").arg(locale.toString(count));
}

QString EditorStatusBar::zoomText(int percent)
{
    return tr("%1%").arg(percent);
}

QString EditorStatusBar::position() const
{
    return m_position->text();
}

QString EditorStatusBar::characters() const
{
    return m_characters->text();
}

QString EditorStatusBar::encoding() const
{
    return m_encoding->text();
}

QString EditorStatusBar::lineEnding() const
{
    return m_lineEnding->text();
}

QString EditorStatusBar::zoom() const
{
    return m_zoom->text();
}

QLabel* EditorStatusBar::addSegment(const QString& accessibleName)
{
    auto* label = new QLabel(this);
    label->setAccessibleName(accessibleName);
    label->setContentsMargins(8, 0, 8, 0);
    addPermanentWidget(label);
    return label;
}

void EditorStatusBar::followCurrentEditor()
{
    EditorWidget* current = m_tabs.currentEditor();
    if (current != m_editor) {
        if (m_editor != nullptr) {
            m_editor->disconnect(this);
        }
        m_editor = current;
        if (m_editor != nullptr) {
            connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
                    &EditorStatusBar::refresh);
            connect(m_editor, &QPlainTextEdit::textChanged, this, &EditorStatusBar::refresh);
        }
    }
    refresh();
}

void EditorStatusBar::refresh()
{
    m_zoom->setText(zoomText(m_tabs.viewSettings().zoomPercent));
    if (m_editor == nullptr) {
        return;
    }

    const QTextCursor cursor = m_editor->textCursor();
    m_position->setText(positionText(cursor.blockNumber() + 1, cursor.positionInBlock() + 1));

    // Qt counts the end of the last paragraph as a character; line breaks inside count as one.
    m_characters->setText(charactersText(m_editor->document()->characterCount() - 1));

    m_encoding->setText(core::encodingName(m_editor->fileDocument().encoding()));
    m_lineEnding->setText(core::lineEndingName(m_editor->fileDocument().lineEnding()));
}

} // namespace scribe::editor
