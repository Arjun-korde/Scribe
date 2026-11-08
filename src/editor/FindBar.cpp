#include "FindBar.h"

#include "EditorSearch.h"
#include "EditorTabs.h"
#include "EditorWidget.h"

#include <QAccessible>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace scribe::editor
{

namespace
{

// A selection longer than this is not turned into the text to find.
constexpr int kMaxPrefillLength = 256;

QToolButton* makeButton(const QString& text, const QString& toolTip, const QString& accessibleName,
                        bool checkable, QWidget* parent)
{
    auto* button = new QToolButton(parent);
    button->setText(text);
    button->setToolTip(toolTip);
    button->setAccessibleName(accessibleName);
    button->setCheckable(checkable);
    button->setAutoRaise(true);
    // Clicking a button must not take the caret out of the Find field or the text.
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

} // namespace

FindBar::FindBar(EditorTabs& tabs, QWidget* parent)
    : QWidget(parent)
    , m_tabs(tabs)
{
    m_find = new QLineEdit(this);
    m_find->setPlaceholderText(tr("Find"));
    m_find->setAccessibleName(tr("Find"));
    m_find->setClearButtonEnabled(true);

    m_matchCase = makeButton(tr("Aa"), tr("Match case"), tr("Match case"), true, this);
    m_wrap = makeButton(tr("Wrap"), tr("Wrap around"), tr("Wrap around"), true, this);
    m_wrap->setChecked(true); // on by default

    m_status = new QLabel(this);
    m_status->setAccessibleName(tr("Search result"));
    m_status->setMinimumWidth(
        m_status->fontMetrics().horizontalAdvance(QStringLiteral("000,000+ results")));

    m_previous =
        makeButton(tr("<"), tr("Previous match (Shift+Enter)"), tr("Previous match"), false, this);
    m_next = makeButton(tr(">"), tr("Next match (Enter)"), tr("Next match"), false, this);
    m_close = makeButton(tr("x"), tr("Close (Esc)"), tr("Close"), false, this);

    auto* findRow = new QHBoxLayout;
    findRow->addWidget(m_find, 1);
    findRow->addWidget(m_matchCase);
    findRow->addWidget(m_wrap);
    findRow->addWidget(m_status);
    findRow->addWidget(m_previous);
    findRow->addWidget(m_next);
    findRow->addWidget(m_close);

    m_replace = new QLineEdit(this);
    m_replace->setPlaceholderText(tr("Replace"));
    m_replace->setAccessibleName(tr("Replace with"));
    m_replaceButton = new QPushButton(tr("Replace"), this);
    m_replaceAllButton = new QPushButton(tr("Replace All"), this);
    m_replaceButton->setAutoDefault(false);
    m_replaceAllButton->setAutoDefault(false);
    m_replaceButton->setFocusPolicy(Qt::NoFocus);
    m_replaceAllButton->setFocusPolicy(Qt::NoFocus);

    m_replaceRow = new QWidget(this);
    auto* replaceLayout = new QHBoxLayout(m_replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->addWidget(m_replace, 1);
    replaceLayout->addWidget(m_replaceButton);
    replaceLayout->addWidget(m_replaceAllButton);
    m_replaceRow->hide();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);
    layout->addLayout(findRow);
    layout->addWidget(m_replaceRow);

    m_find->installEventFilter(this);
    m_replace->installEventFilter(this);

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(100);
    connect(&m_refreshTimer, &QTimer::timeout, this, &FindBar::updateStatus);

    connect(m_find, &QLineEdit::textChanged, this, &FindBar::onQueryChanged);
    connect(m_matchCase, &QToolButton::toggled, this, [this] {
        m_message.clear();
        m_matchesValid = false;
        searchFromOrigin();
    });
    connect(m_wrap, &QToolButton::toggled, this, [this] { m_message.clear(); });
    connect(m_previous, &QToolButton::clicked, this, [this] { findPrevious(); });
    connect(m_next, &QToolButton::clicked, this, [this] { findNext(); });
    connect(m_close, &QToolButton::clicked, this, &FindBar::closeBar);
    connect(m_replaceButton, &QPushButton::clicked, this, [this] { replaceCurrent(); });
    connect(m_replaceAllButton, &QPushButton::clicked, this, [this] { replaceAll(); });
    connect(m_replace, &QLineEdit::textChanged, this, [this] { m_message.clear(); });

    connect(&m_tabs, &EditorTabs::currentEditorStateChanged, this, &FindBar::followCurrentEditor);

    hide();
    followCurrentEditor();
    updateStatus();
}

// ---- Opening and closing --------------------------------------------------------------------

void FindBar::openFind()
{
    showBar();

    // The search starts where the caret (or selection) is now, before anything is typed.
    if (m_editor) {
        remember(m_editor->textCursor());

        // A one-line selection becomes the text to find, like in most editors.
        const QTextCursor cursor = m_editor->textCursor();
        const QString selected = cursor.selectedText();
        if (cursor.hasSelection() && selected.size() <= kMaxPrefillLength &&
            !selected.contains(QChar(0x2029))) {
            setQuery(selected);
        }
    }

    m_find->setFocus();
    m_find->selectAll();
    updateStatus();
}

void FindBar::openReplace()
{
    setReplaceRowVisible(true);
    openFind();
}

void FindBar::closeBar()
{
    if (!isOpen()) {
        return;
    }
    hide();
    m_message.clear();
    if (m_editor) {
        EditorSearch::highlight(*m_editor, std::nullopt);
        m_editor->setFocus(); // back to the text, with its selection kept
    }
}

void FindBar::showBar()
{
    show();
}

void FindBar::setReplaceRowVisible(bool visible)
{
    m_replaceRow->setVisible(visible);
}

bool FindBar::isReplaceRowOpen() const
{
    return isOpen() && !m_replaceRow->isHidden();
}

// ---- State ----------------------------------------------------------------------------------

QString FindBar::query() const
{
    return m_find->text();
}

void FindBar::setQuery(const QString& text)
{
    m_find->setText(text);
}

QString FindBar::replacement() const
{
    return m_replace->text();
}

void FindBar::setReplacement(const QString& text)
{
    m_replace->setText(text);
}

bool FindBar::matchCase() const
{
    return m_matchCase->isChecked();
}

void FindBar::setMatchCase(bool on)
{
    m_matchCase->setChecked(on);
}

bool FindBar::wrapAround() const
{
    return m_wrap->isChecked();
}

void FindBar::setWrapAround(bool on)
{
    m_wrap->setChecked(on);
}

QString FindBar::statusText() const
{
    return m_status->text();
}

core::SearchOptions FindBar::options() const
{
    core::SearchOptions options;
    options.matchCase = matchCase();
    options.wrapAround = wrapAround();
    return options;
}

// ---- Wording --------------------------------------------------------------------------------

QString FindBar::countText(qsizetype index, qsizetype total, bool capped, const QLocale& locale)
{
    if (capped) {
        return tr("%1+ results").arg(locale.toString(kMaxCountedMatches));
    }
    if (index > 0) {
        return tr("%1 of %2").arg(locale.toString(index), locale.toString(total));
    }
    return total == 1 ? tr("1 result") : tr("%1 results").arg(locale.toString(total));
}

QString FindBar::replacedText(qsizetype count, const QLocale& locale)
{
    return tr("%1 replaced").arg(locale.toString(count));
}

QString FindBar::noResultsText()
{
    return tr("No results");
}

// ---- Following the active tab ---------------------------------------------------------------

void FindBar::followCurrentEditor()
{
    EditorWidget* current = m_tabs.currentEditor();
    if (current == m_editor) {
        return;
    }

    if (m_editor) {
        EditorSearch::highlight(*m_editor, std::nullopt);
        m_editor->disconnect(this);
        m_editor->document()->disconnect(this);
    }

    m_editor = current;
    m_matchesValid = false;
    m_message.clear();

    if (m_editor) {
        connect(m_editor->document(), &QTextDocument::contentsChanged, this,
                &FindBar::onContentsChanged);
        connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &FindBar::onCursorMoved);
        remember(m_editor->textCursor());
    }
    updateStatus();
}

void FindBar::onContentsChanged()
{
    m_matchesValid = false; // the text changed, so the cached matches are stale
    if (!m_changingEditor) {
        m_message.clear(); // the user edited the text: "12 replaced" no longer applies
        m_refreshTimer.start();
    }
}

void FindBar::onCursorMoved()
{
    if (m_changingEditor || !m_editor) {
        return;
    }
    // The user moved the caret: the next incremental search starts from there.
    remember(m_editor->textCursor());
    m_refreshTimer.start();
}

void FindBar::remember(const QTextCursor& cursor)
{
    m_origin = cursor.hasSelection() ? cursor.selectionStart() : cursor.position();
}

// ---- Searching ------------------------------------------------------------------------------

void FindBar::ensureMatches()
{
    if (m_matchesValid || !m_editor) {
        return;
    }
    m_text = m_editor->toPlainText();

    // The list is only for counting, so it never needs wrap-around.
    core::SearchOptions counting = options();
    counting.wrapAround = false;
    m_matches = core::findAll(m_text, query(), counting, kMaxCountedMatches + 1);
    m_capped = m_matches.size() > kMaxCountedMatches;
    if (m_capped) {
        m_matches.removeLast();
    }
    m_matchesValid = true;
}

void FindBar::selectMatch(const core::TextRange& range)
{
    m_changingEditor = true;
    EditorSearch::select(*m_editor, range);
    EditorSearch::highlight(*m_editor, range);
    m_changingEditor = false;
}

void FindBar::onQueryChanged()
{
    m_message.clear();
    m_matchesValid = false;
    searchFromOrigin();
}

void FindBar::searchFromOrigin()
{
    if (!m_editor) {
        updateStatus();
        return;
    }
    if (!hasQuery()) {
        EditorSearch::highlight(*m_editor, std::nullopt);
        updateStatus();
        return;
    }

    // Incremental search: look from where the search started, not from the last match, so that
    // deleting a letter goes back to the match that was current before.
    ensureMatches();
    if (const auto match = core::findNext(m_text, query(), m_origin, options())) {
        selectMatch(*match);
    } else {
        EditorSearch::highlight(*m_editor, std::nullopt);
    }
    updateStatus();
}

bool FindBar::findNext()
{
    if (!m_editor) {
        return false;
    }
    if (!hasQuery()) {
        openFind();
        return false;
    }

    m_message.clear();
    ensureMatches();
    const QTextCursor cursor = m_editor->textCursor();
    const qsizetype from = cursor.hasSelection() ? cursor.selectionEnd() : cursor.position();

    if (const auto match = core::findNext(m_text, query(), from, options())) {
        selectMatch(*match);
        m_origin = match->position;
        updateStatus();
        return true;
    }

    m_message = m_matches.isEmpty() ? QString() : tr("No more results");
    if (!isOpen()) {
        showBar(); // so the user can see why nothing happened
    }
    updateStatus();
    return false;
}

bool FindBar::findPrevious()
{
    if (!m_editor) {
        return false;
    }
    if (!hasQuery()) {
        openFind();
        return false;
    }

    m_message.clear();
    ensureMatches();
    const QTextCursor cursor = m_editor->textCursor();
    const qsizetype before = cursor.hasSelection() ? cursor.selectionStart() : cursor.position();

    if (const auto match = core::findPrevious(m_text, query(), before, options())) {
        selectMatch(*match);
        m_origin = match->position;
        updateStatus();
        return true;
    }

    m_message = m_matches.isEmpty() ? QString() : tr("No more results");
    if (!isOpen()) {
        showBar();
    }
    updateStatus();
    return false;
}

// ---- Replacing ------------------------------------------------------------------------------

bool FindBar::replaceCurrent()
{
    if (!m_editor || !hasQuery()) {
        return false;
    }

    bool replaced = false;
    if (EditorSearch::selectionIsMatch(*m_editor, query(), options())) {
        m_changingEditor = true;
        EditorSearch::replaceSelection(*m_editor, replacement());
        m_changingEditor = false;
        m_matchesValid = false;
        replaced = true;
    }

    // The caret is now after the replacement, so this cannot find the text just inserted.
    findNext();
    return replaced;
}

qsizetype FindBar::replaceAll()
{
    if (!m_editor || !hasQuery()) {
        return 0;
    }

    // A destructive edit always works on the text as it is now, never on a cached copy.
    const QString text = m_editor->toPlainText();
    const QList<core::TextRange> ranges = core::findAll(text, query(), options());

    m_matchesValid = false;
    if (ranges.isEmpty()) {
        m_message.clear();
        updateStatus(); // "No results"
        return 0;
    }

    m_changingEditor = true;
    const qsizetype count = EditorSearch::replaceRanges(*m_editor, ranges, replacement());
    EditorSearch::highlight(*m_editor, std::nullopt);
    m_changingEditor = false;

    remember(m_editor->textCursor());
    m_matchesValid = false;
    m_message = replacedText(count);
    updateStatus();
    return count;
}

// ---- Status text ----------------------------------------------------------------------------

QString FindBar::computeStatus()
{
    if (!m_message.isEmpty()) {
        return m_message;
    }
    if (!hasQuery() || !m_editor) {
        return {};
    }

    ensureMatches();
    if (m_matches.isEmpty()) {
        return noResultsText();
    }
    if (m_capped) {
        return countText(0, m_matches.size(), true);
    }

    // "3 of 12" when the selection is one of the matches, otherwise just the count.
    qsizetype index = 0;
    const QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection()) {
        const qsizetype start = cursor.selectionStart();
        const auto it = std::lower_bound(m_matches.cbegin(), m_matches.cend(), start,
                                         [](const core::TextRange& match, qsizetype position) {
                                             return match.position < position;
                                         });
        if (it != m_matches.cend() && it->position == start &&
            it->length == cursor.selectionEnd() - start) {
            index = (it - m_matches.cbegin()) + 1;
        }
    }
    return countText(index, m_matches.size(), false);
}

void FindBar::updateStatus()
{
    const QString text = computeStatus();
    const bool changed = text != m_status->text();
    m_status->setText(text);

    // For the theme to style later, and so that "no results" is never shown by color alone.
    m_status->setProperty("noResults", text == noResultsText());

    const bool hasText = hasQuery();
    m_replaceButton->setEnabled(hasText);
    m_replaceAllButton->setEnabled(hasText);

    if (changed && !text.isEmpty()) {
        QAccessibleEvent alert(m_status, QAccessible::Alert);
        QAccessible::updateAccessibility(&alert);
    }
}

// ---- Keys -----------------------------------------------------------------------------------

void FindBar::keyPressEvent(QKeyEvent* event)
{
    // Esc from anywhere in the bar (the fields handle their own, see eventFilter()).
    if (event->key() == Qt::Key_Escape) {
        closeBar();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool FindBar::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_find || watched == m_replace) && event->type() == QEvent::KeyPress) {
        const auto* key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
            case Qt::Key_Escape:
                closeBar();
                return true;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (watched == m_replace) {
                    replaceCurrent();
                } else if (key->modifiers() & Qt::ShiftModifier) {
                    findPrevious();
                } else {
                    findNext();
                }
                return true;
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace scribe::editor
