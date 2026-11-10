#pragma once

#include <QTabBar>

namespace scribe::editor
{

/// The strip of tabs, drawn as in the design: 32 px tabs 168 px wide with the file name aligned to
/// the left, a small "x" button on the right, and a rounded top. The window style cannot do this
/// (it centres the text and draws its own close button), so the tabs are painted here, using the
/// application palette so that a theme change needs nothing else.
///
/// Closing is reported like QTabBar does: through tabCloseRequested(index).
class EditorTabBar : public QTabBar
{
    Q_OBJECT

public:
    static constexpr int kTabWidth = 168;
    static constexpr int kTabHeight = 32;
    static constexpr int kTopGap = 4;     ///< Space above the tabs, inside the strip.
    static constexpr int kTabSpacing = 2; ///< Space between tabs.
    static constexpr int kLeadIn = 8;     ///< Space before the first tab.

    explicit EditorTabBar(QWidget* parent = nullptr);

    QSize tabSizeHint(int index) const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void tabInserted(int index) override;
    void tabLayoutChange() override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    /// The tab's visible shape: its rect without the top gap, the spacing and, for the first tab,
    /// the lead-in.
    QRect bodyRect(int index) const;

    int m_hoveredTab = -1;
};

} // namespace scribe::editor
