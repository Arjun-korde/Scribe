#include "EditorTabs.h"

#include "EditorTabBar.h"
#include "EditorWidget.h"
#include "FileDropFilter.h"

#include <QDir>
#include <QFileInfo>
#include <QMouseEvent>
#include <QTabBar>
#include <QToolButton>

namespace scribe::editor
{

namespace
{
constexpr int kNewTabButtonSize = 28; // also the minimum click target in the design
} // namespace

EditorTabs::EditorTabs(QWidget* parent)
    : QTabWidget(parent)
{
    setTabBar(new EditorTabBar(this)); // draws the tabs and their close buttons
    // QTabWidget forwards the bar's close requests only for the bar's own (tabsClosable) buttons.
    connect(tabBar(), &QTabBar::tabCloseRequested, this, &QTabWidget::tabCloseRequested);
    setDocumentMode(true);
    setMovable(true);
    tabBar()->installEventFilter(this);

    m_newTabButton = new QToolButton(tabBar());
    m_newTabButton->setObjectName(QStringLiteral("newTabButton")); // styled by the theme
    m_newTabButton->setText(QStringLiteral("+"));
    m_newTabButton->setToolTip(tr("New tab"));
    m_newTabButton->setAccessibleName(tr("New tab"));
    m_newTabButton->setFocusPolicy(Qt::NoFocus); // a click must not take the focus from the text
    m_newTabButton->setFixedSize(kNewTabButtonSize, kNewTabButtonSize);
    connect(m_newTabButton, &QToolButton::clicked, this, [this] { addEditor(); });
    connect(tabBar(), &QTabBar::tabMoved, this, &EditorTabs::positionNewTabButton);

    // Files dragged from Explorer can land on the tab strip (this widget) or on an editor's text
    // area (see adopt()). The tab bar itself does not take drops, so Qt hands them to this widget.
    m_dropFilter = new FileDropFilter(this);
    setAcceptDrops(true);
    installEventFilter(m_dropFilter);
    connect(m_dropFilter, &FileDropFilter::filesDropped, this, &EditorTabs::filesDropped);

    connect(this, &QTabWidget::tabCloseRequested, this, &EditorTabs::closeTab);
    connect(this, &QTabWidget::currentChanged, this, [this] {
        if (EditorWidget* editor = currentEditor()) {
            editor->setFocus();
        }
        emit currentEditorStateChanged();
    });

    addEditor();
}

EditorWidget* EditorTabs::currentEditor() const
{
    return qobject_cast<EditorWidget*>(currentWidget());
}

EditorWidget* EditorTabs::editorAt(int index) const
{
    return qobject_cast<EditorWidget*>(widget(index));
}

EditorWidget* EditorTabs::addEditor()
{
    return adopt(std::make_unique<EditorWidget>());
}

int EditorTabs::indexOfPath(const QString& path) const
{
    if (path.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < count(); ++i) {
        const QString& tabPath = editorAt(i)->fileDocument().filePath();
        if (!tabPath.isEmpty() && QFileInfo(tabPath) == QFileInfo(path)) {
            return i;
        }
    }
    return -1;
}

core::IoResult EditorTabs::openFile(const QString& path)
{
    if (const int existing = indexOfPath(path); existing >= 0) {
        setCurrentIndex(existing);
        emit fileUsed(path);
        return {};
    }

    if (EditorWidget* current = currentEditor(); isUntouchedUntitled(*current)) {
        return current->openFile(path);
    }

    // Load into a detached editor first, so a failure leaves no half-made tab behind.
    auto editor = std::make_unique<EditorWidget>();
    const core::IoResult result = editor->openFile(path);
    if (result.ok()) {
        adopt(std::move(editor));
        emit fileUsed(path); // the editor loaded the file before it was connected
    }
    return result;
}

void EditorTabs::setCloseGuard(CloseGuard guard)
{
    m_closeGuard = std::move(guard);
}

bool EditorTabs::closeTab(int index)
{
    if (index < 0 || index >= count()) {
        return false;
    }

    QWidget* page = widget(index);
    if (!confirmClose(page)) {
        return false;
    }
    if (count() == 1) {
        addEditor(); // before removing, so there is never a moment without an editor
    }
    removeAndDelete(page);
    return true;
}

bool EditorTabs::closeOtherTabs(int index)
{
    if (index < 0 || index >= count()) {
        return false;
    }

    QWidget* keep = widget(index);
    for (QWidget* page : pages()) {
        if (page == keep) {
            continue;
        }
        if (!confirmClose(page)) {
            return false;
        }
        removeAndDelete(page);
    }
    setCurrentWidget(keep);
    return true;
}

bool EditorTabs::closeAllTabs()
{
    for (QWidget* page : pages()) {
        if (!confirmClose(page)) {
            return false;
        }
        if (count() == 1) {
            addEditor(); // the last tab is replaced by a fresh one, never left empty
        }
        removeAndDelete(page);
    }
    return true;
}

void EditorTabs::nextTab()
{
    if (count() > 1) {
        setCurrentIndex((currentIndex() + 1) % count());
    }
}

void EditorTabs::previousTab()
{
    if (count() > 1) {
        setCurrentIndex((currentIndex() + count() - 1) % count());
    }
}

void EditorTabs::setViewSettings(const ViewSettings& settings)
{
    ViewSettings clamped = settings;
    clamped.zoomPercent = ViewSettings::clampZoom(clamped.zoomPercent);
    changeViewSettings(clamped);
}

void EditorTabs::setStatusBarVisible(bool visible)
{
    ViewSettings settings = m_viewSettings;
    settings.statusBarVisible = visible;
    changeViewSettings(settings);
}

void EditorTabs::setWordWrap(bool wrap)
{
    ViewSettings settings = m_viewSettings;
    settings.wordWrap = wrap;
    changeViewSettings(settings);
}

void EditorTabs::setBaseFont(const QFont& font)
{
    ViewSettings settings = m_viewSettings;
    settings.font = font;
    changeViewSettings(settings);
}

void EditorTabs::setZoomPercent(int percent)
{
    ViewSettings settings = m_viewSettings;
    settings.zoomPercent = ViewSettings::clampZoom(percent);
    changeViewSettings(settings);
}

void EditorTabs::zoomBy(int steps)
{
    setZoomPercent(m_viewSettings.zoomPercent + steps * ViewSettings::kZoomStepPercent);
}

void EditorTabs::resetZoom()
{
    setZoomPercent(ViewSettings::kDefaultZoomPercent);
}

void EditorTabs::applyViewSettings(EditorWidget& editor) const
{
    editor.setLineWrapMode(m_viewSettings.wordWrap ? QPlainTextEdit::WidgetWidth
                                                   : QPlainTextEdit::NoWrap);
    editor.setFont(m_viewSettings.zoomedFont());
}

void EditorTabs::changeViewSettings(const ViewSettings& settings)
{
    if (settings == m_viewSettings) {
        return;
    }
    m_viewSettings = settings;
    for (int i = 0; i < count(); ++i) {
        applyViewSettings(*editorAt(i));
    }
    emit viewSettingsChanged();
}

void EditorTabs::tabInserted(int index)
{
    QTabWidget::tabInserted(index);
    positionNewTabButton();
}

void EditorTabs::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);
    positionNewTabButton();
}

void EditorTabs::positionNewTabButton()
{
    if (m_newTabButton == nullptr || count() == 0) {
        return;
    }
    // The tab sizes are worked out on the next layout pass, so wait for it.
    QMetaObject::invokeMethod(
        this,
        [this] {
            const QRect last = tabBar()->tabRect(count() - 1);
            // Centred on the tab's body (below the gap at the top); the tab rect already ends
            // with the spacing between tabs.
            const int y = last.top() + EditorTabBar::kTopGap +
                          (EditorTabBar::kTabHeight - kNewTabButtonSize) / 2;
            m_newTabButton->move(last.right() + 1, y);
            m_newTabButton->raise();
        },
        Qt::QueuedConnection);
}

bool EditorTabs::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == tabBar() &&
        (event->type() == QEvent::LayoutRequest || event->type() == QEvent::Resize)) {
        positionNewTabButton(); // a tab got longer or shorter, or the strip was resized
    }
    if (watched == tabBar() && event->type() == QEvent::MouseButtonRelease) {
        const auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::MiddleButton) {
            const int index = tabBar()->tabAt(mouse->position().toPoint());
            if (index >= 0) {
                closeTab(index);
                return true;
            }
        }
    }
    return QTabWidget::eventFilter(watched, event);
}

EditorWidget* EditorTabs::adopt(std::unique_ptr<EditorWidget> editor)
{
    EditorWidget* raw = editor.release(); // the tab widget owns it from here on

    // The text area takes drops itself, so file drops must be caught there before it inserts them.
    raw->viewport()->setAcceptDrops(true);
    raw->viewport()->installEventFilter(m_dropFilter);

    applyViewSettings(*raw);
    connect(raw, &EditorWidget::zoomRequested, this, &EditorTabs::zoomBy);
    connect(raw, &EditorWidget::fileUsed, this, &EditorTabs::fileUsed);
    connect(raw, &EditorWidget::stateChanged, this, [this, raw] {
        refreshTab(raw);
        if (raw == currentEditor()) {
            emit currentEditorStateChanged();
        }
    });

    const int index = addTab(raw, QString());
    refreshTab(raw);
    setCurrentIndex(index);
    return raw;
}

void EditorTabs::refreshTab(const EditorWidget* editor)
{
    const int index = indexOf(editor);
    if (index < 0) {
        return; // not in a tab yet
    }

    // '&' would be read as a keyboard mnemonic by the tab bar.
    QString text = editor->displayName();
    text.replace(QLatin1Char('&'), QStringLiteral("&&"));
    if (editor->isModified()) {
        text += QStringLiteral(" *");
    }
    setTabText(index, text);

    const QString& path = editor->fileDocument().filePath();
    setTabToolTip(index, path.isEmpty() ? editor->displayName() : QDir::toNativeSeparators(path));
}

void EditorTabs::removeAndDelete(QWidget* page)
{
    removeTab(indexOf(page));
    page->deleteLater();
}

QList<QWidget*> EditorTabs::pages() const
{
    QList<QWidget*> result;
    for (int i = 0; i < count(); ++i) {
        result.append(widget(i));
    }
    return result;
}

bool EditorTabs::confirmClose(QWidget* page)
{
    auto* editor = qobject_cast<EditorWidget*>(page);
    if (editor == nullptr || !editor->isModified() || !m_closeGuard) {
        return true;
    }

    setCurrentWidget(page);
    return m_closeGuard(*editor);
}

bool EditorTabs::isUntouchedUntitled(const EditorWidget& editor)
{
    return editor.fileDocument().filePath().isEmpty() && !editor.isModified() &&
           editor.document()->isEmpty();
}

} // namespace scribe::editor
