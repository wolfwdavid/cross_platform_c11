#pragma once

#include <QUuid>
#include <memory>
#include <variant>
#include <vector>

namespace c11 {

// Recursive binary split tree replacing Bonsplit.
// Each node is either a Leaf (single panel) or a Split (two children).
class PaneLayout {
public:
    enum class Direction { Horizontal, Vertical };

    struct Leaf {
        QUuid panelId;
    };

    struct Split {
        Direction direction;
        double ratio = 0.5; // 0.0–1.0, position of the divider
        std::unique_ptr<PaneLayout> first;
        std::unique_ptr<PaneLayout> second;
    };

    // Constructors
    static std::unique_ptr<PaneLayout> makeLeaf(const QUuid &panelId);
    static std::unique_ptr<PaneLayout> makeSplit(Direction dir,
                                                  std::unique_ptr<PaneLayout> first,
                                                  std::unique_ptr<PaneLayout> second,
                                                  double ratio = 0.5);

    bool isLeaf() const { return std::holds_alternative<Leaf>(m_node); }
    bool isSplit() const { return std::holds_alternative<Split>(m_node); }

    const Leaf &leaf() const { return std::get<Leaf>(m_node); }
    const Split &split() const { return std::get<Split>(m_node); }
    Split &split() { return std::get<Split>(m_node); }

    // Find the leaf containing panelId
    PaneLayout *findLeaf(const QUuid &panelId);
    const PaneLayout *findLeaf(const QUuid &panelId) const;

    // Find parent split of a given panelId, returns nullptr if root is the leaf
    PaneLayout *findParent(const QUuid &panelId);

    // Split a leaf into two panels
    bool splitLeaf(const QUuid &existingPanelId,
                   const QUuid &newPanelId,
                   Direction direction,
                   bool insertAfter = true);

    // Remove a panel, collapsing its parent split
    bool removePanel(const QUuid &panelId);

    // Collect all panel IDs in order (depth-first, first-before-second)
    std::vector<QUuid> allPanelIds() const;

    // Count leaves
    int leafCount() const;

private:
    std::variant<Leaf, Split> m_node;
};

} // namespace c11
