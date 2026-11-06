#pragma once

#include <QLocale>
#include <QPointer>
#include <QStatusBar>

class QLabel;

namespace scribe::editor
{

class EditorTabs;
class EditorWidget;

/// The bar under the editor: `Ln 3, Col 14 | 1,204 chars | UTF-8 | CRLF | 100%`.
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

private:
    QLabel* addSegment(const QString& accessibleName);

    /// Starts following the active tab's editor (and stops following the previous one).
    void followCurrentEditor();

    /// Re-reads everything from the followed editor and the shared view settings.
    void refresh();

    EditorTabs& m_tabs;
    QPointer<EditorWidget> m_editor;

    QLabel* m_position = nullptr;
    QLabel* m_characters = nullptr;
    QLabel* m_encoding = nullptr;
    QLabel* m_lineEnding = nullptr;
    QLabel* m_zoom = nullptr;
};

} // namespace scribe::editor
