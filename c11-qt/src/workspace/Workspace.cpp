#include "Workspace.h"

namespace c11 {

Workspace::Workspace(GhosttyRuntime &runtime,
                     const QString &title,
                     const QString &workingDirectory,
                     QObject *parent)
    : QObject(parent)
    , m_id(QUuid::createUuid())
    , m_title(title)
    , m_runtime(runtime)
{
    // Create initial terminal panel
    auto *panel = createTerminalPanel(workingDirectory);
    m_focusedPanelId = panel->id();
}

Workspace::~Workspace()
{
    qDeleteAll(m_panels);
}

void Workspace::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged(effectiveTitle());
    }
}

void Workspace::setCustomTitle(const QString &title)
{
    m_customTitle = title;
    emit titleChanged(effectiveTitle());
}

QString Workspace::effectiveTitle() const
{
    return m_customTitle.isEmpty() ? m_title : m_customTitle;
}

void Workspace::setPinned(bool pinned)
{
    if (m_pinned != pinned) {
        m_pinned = pinned;
        emit pinnedChanged(pinned);
    }
}

TerminalPanel *Workspace::createTerminalPanel(const QString &workingDirectory,
                                               const QString &command)
{
    auto *panel = new TerminalPanel(m_runtime, m_id, workingDirectory, command, this);
    m_panels.insert(panel->id(), panel);

    if (!m_layout) {
        m_layout = PaneLayout::makeLeaf(panel->id());
    }

    connect(panel, &Panel::titleChanged, this, [this, panel]() {
        if (panel->id() == m_focusedPanelId) {
            setTitle(panel->displayTitle());
        }
    });

    emit panelAdded(panel->id());
    emit layoutChanged();
    return panel;
}

void Workspace::removePanel(const QUuid &panelId)
{
    auto *panel = m_panels.value(panelId);
    if (!panel) return;

    panel->close();
    m_panels.remove(panelId);

    if (m_layout) {
        m_layout->removePanel(panelId);
    }

    // Update focus if the removed panel was focused
    if (m_focusedPanelId == panelId) {
        auto ids = orderedPanelIds();
        m_focusedPanelId = ids.empty() ? QUuid() : ids.front();
        emit focusedPanelChanged(m_focusedPanelId);
    }

    emit panelRemoved(panelId);
    emit layoutChanged();
    delete panel;
}

void Workspace::splitPanel(const QUuid &existingPanelId,
                            PaneLayout::Direction direction,
                            const QString &workingDirectory,
                            bool insertAfter)
{
    if (!m_layout || !m_layout->findLeaf(existingPanelId)) return;

    auto *newPanel = createTerminalPanel(workingDirectory);
    m_layout->splitLeaf(existingPanelId, newPanel->id(), direction, insertAfter);

    setFocusedPanelId(newPanel->id());
    emit layoutChanged();
}

void Workspace::setFocusedPanelId(const QUuid &id)
{
    if (m_focusedPanelId != id && m_panels.contains(id)) {
        if (auto *old = m_panels.value(m_focusedPanelId)) {
            old->unfocus();
        }
        m_focusedPanelId = id;
        if (auto *p = m_panels.value(id)) {
            p->focus();
        }
        emit focusedPanelChanged(id);
    }
}

std::vector<QUuid> Workspace::orderedPanelIds() const
{
    return m_layout ? m_layout->allPanelIds() : std::vector<QUuid>{};
}

} // namespace c11
