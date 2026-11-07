#pragma once

#include <QLocale>
#include <QPointer>
#include <QStatusBar>

class QLabel;
class QToolButton;

namespace scribe::editor
{

class EditorTabs;
class EditorWidget;

/// The bar under the editor: `Ln 3, Col 14 | 1,204 chars | UTF-8 | CRLF | 100%`. The encoding and
/// line ending segments are buttons: clicking one reports it so the window can show its choices.
/// It always describes the active tab and updates as the user types, moves the caret, switches
/// tabs, or changes the encoding, line ending or zoom.
class EditorStatusBar : public QStatusBar
{
    Q_OBJECT

public:
    explicit EditorStatusBar(EditorTabs& tabs, QWidget* parent = nullptr);

    // The wording of each segment, public so that tests (and later the command palette) can use it.
    static QString positionText(int line, int column);
    static QString charactersText(qsizetype count, const QLocale& locale = QLocale::system());
    static QString zoomText(int percent);

    // What each segment currently shows.
    [[nodiscard]] QString position() const;
    [[nodiscard]] QString characters() const;
    [[nodiscard]] QString encoding() const;
    [[nodiscard]] QString lineEnding() const;
    [[nodiscard]] QString zoom() const;

signals:
    /// The encoding / line ending segment was clicked. `anchor` is the segment, so that the
    /// receiver can open a menu next to it.
    void encodingClicked(QWidget* anchor);
    void lineEndingClicked(QWidget* anchor);

private:
    QLabel* addSegment(const QString& accessibleName);
    QToolButton* addButtonSegment(const QString& accessibleName, const QString& toolTip);

    /// Starts following the active tab's editor (and stops following the previous one).
    void followCurrentEditor();

    /// Re-reads everything from the followed editor and the shared view settings.
    void refresh();

    EditorTabs& m_tabs;
    QPointer<EditorWidget> m_editor;

    QLabel* m_position = nullptr;
    QLabel* m_characters = nullptr;
    QToolButton* m_encoding = nullptr;
    QToolButton* m_lineEnding = nullptr;
    QLabel* m_zoom = nullptr;
};

} // namespace scribe::editor
