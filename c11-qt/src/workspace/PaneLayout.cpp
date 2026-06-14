#include "PaneLayout.h"

namespace c11 {

std::unique_ptr<PaneLayout> PaneLayout::makeLeaf(const QUuid &panelId)
{
    auto layout = std::make_unique<PaneLayout>();
    layout->m_node = Leaf{panelId};
    return layout;
}

std::unique_ptr<PaneLayout> PaneLayout::makeSplit(Direction dir,
                                                   std::unique_ptr<PaneLayout> first,
                                                   std::unique_ptr<PaneLayout> second,
                                                   double ratio)
{
    auto layout = std::make_unique<PaneLayout>();
    layout->m_node = Split{dir, ratio, std::move(first), std::move(second)};
    return layout;
}

PaneLayout *PaneLayout::findLeaf(const QUuid &panelId)
{
    if (isLeaf()) {
        return leaf().panelId == panelId ? this : nullptr;
    }
    auto &s = split();
    if (auto *found = s.first->findLeaf(panelId)) return found;
    return s.second->findLeaf(panelId);
}

const PaneLayout *PaneLayout::findLeaf(const QUuid &panelId) const
{
    return const_cast<PaneLayout *>(this)->findLeaf(panelId);
}

PaneLayout *PaneLayout::findParent(const QUuid &panelId)
{
    if (isLeaf()) return nullptr;

    auto &s = split();
    // Check if either child is the target leaf
    if (s.first->isLeaf() && s.first->leaf().panelId == panelId) return this;
    if (s.second->isLeaf() && s.second->leaf().panelId == panelId) return this;

    // Recurse
    if (auto *found = s.first->findParent(panelId)) return found;
    return s.second->findParent(panelId);
}

bool PaneLayout::splitLeaf(const QUuid &existingPanelId,
                            const QUuid &newPanelId,
                            Direction direction,
                            bool insertAfter)
{
    if (isLeaf()) {
        if (leaf().panelId != existingPanelId) return false;

        auto existing = makeLeaf(existingPanelId);
        auto newLeaf = makeLeaf(newPanelId);

        if (insertAfter) {
            m_node = Split{direction, 0.5, std::move(existing), std::move(newLeaf)};
        } else {
            m_node = Split{direction, 0.5, std::move(newLeaf), std::move(existing)};
        }
        return true;
    }

    auto &s = split();
    if (s.first->splitLeaf(existingPanelId, newPanelId, direction, insertAfter)) return true;
    return s.second->splitLeaf(existingPanelId, newPanelId, direction, insertAfter);
}

bool PaneLayout::removePanel(const QUuid &panelId)
{
    if (isLeaf()) return false;

    auto &s = split();

    // Check if first child is the target
    if (s.first->isLeaf() && s.first->leaf().panelId == panelId) {
        // Replace this node with second child
        auto replacement = std::move(s.second);
        m_node = std::move(replacement->m_node);
        return true;
    }

    // Check if second child is the target
    if (s.second->isLeaf() && s.second->leaf().panelId == panelId) {
        auto replacement = std::move(s.first);
        m_node = std::move(replacement->m_node);
        return true;
    }

    // Recurse
    if (s.first->removePanel(panelId)) return true;
    return s.second->removePanel(panelId);
}

std::vector<QUuid> PaneLayout::allPanelIds() const
{
    std::vector<QUuid> result;
    if (isLeaf()) {
        result.push_back(leaf().panelId);
    } else {
        auto first = split().first->allPanelIds();
        auto second = split().second->allPanelIds();
        result.insert(result.end(), first.begin(), first.end());
        result.insert(result.end(), second.begin(), second.end());
    }
    return result;
}

int PaneLayout::leafCount() const
{
    if (isLeaf()) return 1;
    return split().first->leafCount() + split().second->leafCount();
}

} // namespace c11
