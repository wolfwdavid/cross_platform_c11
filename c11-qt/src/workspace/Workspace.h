#pragma once

#include "PaneLayout.h"
#include "panel/Panel.h"
#include "panel/TerminalPanel.h"
#include "panel/BrowserPanel.h"
#include "panel/MarkdownPanel.h"
#include "ghostty/GhosttyRuntime.h"

#include <QObject>
#include <QUuid>
#include <QString>
#include <QMap>
#include <memory>

namespace c11 {

class Workspace : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

public:
    explicit Workspace(GhosttyRuntime &runtime,
                       const QString &title = "Terminal",
                       const QString &workingDirectory = {},
                       QObject *parent = nullptr,
                       bool withInitialPanel = true);
    ~Workspace() override;

    QUuid id() const { return m_id; }
    QString title() const { return m_title; }
    void setTitle(const QString &title);
    QString customTitle() const { return m_customTitle; }
    void setCustomTitle(const QString &title);
    QString effectiveTitle() const;

    bool isPinned() const { return m_pinned; }
    void setPinned(bool pinned);

    // Panel management
    Panel *panel(const QUuid &id) const { return m_panels.value(id); }
    QList<Panel *> allPanels() const { return m_panels.values(); }
    int panelCount() const { return m_panels.size(); }

    TerminalPanel *createTerminalPanel(const QString &workingDirectory = {},
                                        const QString &command = {});
    BrowserPanel *createBrowserPanel(const QUrl &url = QUrl("about:blank"));
    MarkdownPanel *createMarkdownPanel(const QString &filePath = {});
    void removePanel(const QUuid &panelId);

    // Layout
    PaneLayout *layout() const { return m_layout.get(); }

    // Replace the layout tree wholesale (used by session restore, which rebuilds
    // the tree from a snapshot after recreating the panels). The panel ids in
    // `layout` must reference panels already added to this workspace.
    void setLayout(std::unique_ptr<PaneLayout> layout);

    void splitPanel(const QUuid &existingPanelId,
                    PaneLayout::Direction direction,
                    const QString &workingDirectory = {},
                    bool insertAfter = true);

    // Focus
    QUuid focusedPanelId() const { return m_focusedPanelId; }
    void setFocusedPanelId(const QUuid &id);
    Panel *focusedPanel() const { return m_panels.value(m_focusedPanelId); }

    // Panel ID list (layout order)
    std::vector<QUuid> orderedPanelIds() const;

signals:
    void titleChanged(const QString &title);
    void panelAdded(const QUuid &panelId);
    void panelRemoved(const QUuid &panelId);
    void focusedPanelChanged(const QUuid &panelId);
    void layoutChanged();
    void pinnedChanged(bool pinned);

private:
    QUuid m_id;
    QString m_title;
    QString m_customTitle;
    bool m_pinned = false;
    QUuid m_focusedPanelId;

    GhosttyRuntime &m_runtime;
    QMap<QUuid, Panel *> m_panels;
    std::unique_ptr<PaneLayout> m_layout;
};

} // namespace c11
