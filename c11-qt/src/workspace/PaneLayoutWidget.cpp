#include "PaneLayoutWidget.h"

#include <QVBoxLayout>
#include <QSplitter>

namespace c11 {

PaneLayoutWidget::PaneLayoutWidget(PanelResolver resolver, QWidget *parent)
    : QWidget(parent)
    , m_resolver(std::move(resolver))
{
    auto *box = new QVBoxLayout(this);
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
}

PaneLayoutWidget::~PaneLayoutWidget() = default;

void PaneLayoutWidget::setLayout(const PaneLayout &layout)
{
    clear();
    m_rootWidget = buildWidget(layout);
    if (m_rootWidget) {
        static_cast<QVBoxLayout *>(this->layout())->addWidget(m_rootWidget);
    }
}

void PaneLayoutWidget::clear()
{
    if (m_rootWidget) {
        // Detach all panel content widgets (leaves) so they aren't deleted with
        // the splitter scaffolding. Panel widgets are owned by their Panels and
        // must survive a layout rebuild (e.g. a split reuses the existing pane).
        auto detachAll = [](QWidget *w, auto &self) -> void {
            if (auto *splitter = qobject_cast<QSplitter *>(w)) {
                for (int i = splitter->count() - 1; i >= 0; --i) {
                    self(splitter->widget(i), self);
                }
            } else if (w) {
                w->setParent(nullptr);
            }
        };
        detachAll(m_rootWidget, detachAll);

        // Only delete the splitter tree. When the root is a single pane,
        // m_rootWidget IS a panel's content widget — deleting it here would
        // destroy the live pane (and its terminal surface) out from under its
        // owning Panel. detachAll already reparented leaves out of the tree.
        if (qobject_cast<QSplitter *>(m_rootWidget)) {
            delete m_rootWidget;
        }
        m_rootWidget = nullptr;
    }
}

QWidget *PaneLayoutWidget::buildWidget(const PaneLayout &node)
{
    if (node.isLeaf()) {
        auto *widget = m_resolver(node.leaf().panelId);
        return widget; // may be nullptr if panel not found
    }

    const auto &s = node.split();
    auto *splitter = new QSplitter(
        s.direction == PaneLayout::Direction::Horizontal
            ? Qt::Horizontal : Qt::Vertical);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(2);

    auto *first = buildWidget(*s.first);
    auto *second = buildWidget(*s.second);

    if (first) splitter->addWidget(first);
    if (second) splitter->addWidget(second);

    // Apply ratio
    if (first && second) {
        int total = (s.direction == PaneLayout::Direction::Horizontal)
                    ? width() : height();
        if (total <= 0) total = 1000; // default before first layout
        int firstSize = static_cast<int>(total * s.ratio);
        int secondSize = total - firstSize;
        splitter->setSizes({firstSize, secondSize});
    }

    return splitter;
}

} // namespace c11
