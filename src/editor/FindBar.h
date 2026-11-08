#pragma once

#include "TextSearch.h"

#include <QList>
#include <QLocale>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTextCursor;
class QToolButton;

namespace scribe::editor
{

class EditorTabs;
class EditorWidget;

/// The find (and replace) bar under the editor:
///
///   Find:    [____________] [Aa] [Wrap]   3 of 12   [<] [>] [x]
///   Replace: [____________] [Replace] [Replace All]
///
/// There is one bar for the whole window; it searches the active tab. Typing jumps to the first
/// match from where the search started (incremental search), Enter / Shift+Enter go to the next /
/// previous match, and Esc closes the bar and returns to the text.
///
/// The bar stays valid when closed: Find Next and Find Previous keep using its text.
class FindBar : public QWidget
{
    Q_OBJECT

public:
    /// Matches are counted up to this many; beyond it the bar says "100,000+ results".
    static constexpr qsizetype kMaxCountedMatches = 100000;

    explicit FindBar(EditorTabs& tabs, QWidget* parent = nullptr);

    /// Show the bar with the focus in the Find field. A one-line selection in the editor becomes
    /// the text to find.
    void openFind();

    /// Like openFind(), with the Replace row shown too.
    void openReplace();

    /// Hide the bar, remove the match mark and give the focus back to the editor.
    void closeBar();

    /// Select the next / previous match. Return whether one was found. With nothing to search for
    /// they open the bar instead; when nothing is found they show the bar so the message is seen.
    bool findNext();
    bool findPrevious();

    /// Replace the current match (when the selection is one) and select the next match. Returns
    /// whether something was replaced.
    bool replaceCurrent();

    /// Replace every match as one undo step. The bar then says how many were replaced. Returns
    /// that number.
    qsizetype replaceAll();

    [[nodiscard]] bool isOpen() const { return !isHidden(); }
    [[nodiscard]] bool isReplaceRowOpen() const;

    [[nodiscard]] QString query() const;
    void setQuery(const QString& text);
    [[nodiscard]] bool hasQuery() const { return !query().isEmpty(); }

    [[nodiscard]] QString replacement() const;
    void setReplacement(const QString& text);

    [[nodiscard]] bool matchCase() const;
    void setMatchCase(bool on);
    [[nodiscard]] bool wrapAround() const;
    void setWrapAround(bool on);

    /// What the bar currently says: "3 of 12", "12 results", "No results", "5 replaced"...
    [[nodiscard]] QString statusText() const;

    // The wording, public for tests.
    static QString countText(qsizetype index, qsizetype total, bool capped,
                             const QLocale& locale = QLocale::system());
    static QString replacedText(qsizetype count, const QLocale& locale = QLocale::system());
    static QString noResultsText();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    [[nodiscard]] core::SearchOptions options() const;

    /// Starts following the active tab's editor (and stops following the previous one).
    void followCurrentEditor();
    void onContentsChanged();
    void onCursorMoved();
    void onQueryChanged();

    /// Makes the match list and the text snapshot current. Cached until the text, the query or
    /// the options change.
    void ensureMatches();
    [[nodiscard]] QString computeStatus();
    void updateStatus();

    void selectMatch(const core::TextRange& range);
    void searchFromOrigin();
    void remember(const QTextCursor& cursor);
    void showBar();
    void setReplaceRowVisible(bool visible);

    EditorTabs& m_tabs;
    QPointer<EditorWidget> m_editor;

    QLineEdit* m_find = nullptr;
    QLineEdit* m_replace = nullptr;
    QToolButton* m_matchCase = nullptr;
    QToolButton* m_wrap = nullptr;
    QToolButton* m_previous = nullptr;
    QToolButton* m_next = nullptr;
    QToolButton* m_close = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_replaceButton = nullptr;
    QPushButton* m_replaceAllButton = nullptr;
    QWidget* m_replaceRow = nullptr;

    // The text the matches were computed from, and the matches themselves.
    QString m_text;
    QList<core::TextRange> m_matches;
    bool m_capped = false;
    bool m_matchesValid = false;

    QString m_message; ///< Overrides the count (for example "12 replaced") until things change.
    qsizetype m_origin = 0; ///< Where incremental search starts, so typing does not drift.
    bool m_changingEditor =
        false;             ///< True while this bar itself moves the caret or edits the text.
    QTimer m_refreshTimer; ///< Delays recounting while the user types in the editor.
};

} // namespace scribe::editor
