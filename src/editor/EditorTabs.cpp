#include "EditorTabs.h"

#include "EditorWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QMouseEvent>
#include <QTabBar>

namespace scribe::editor
{

EditorTabs::EditorTabs(QWidget* parent)
    : QTabWidget(parent)
{
    setDocumentMode(true);
    setTabsClosable(true);
    setMovable(true);
    tabBar()->installEventFilter(this);

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

bool EditorTabs::eventFilter(QObject* watched, QEvent* event)
{
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
