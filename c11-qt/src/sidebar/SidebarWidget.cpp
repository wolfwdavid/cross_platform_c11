#include "SidebarWidget.h"

#include <QVBoxLayout>
#include <QPushButton>

namespace c11 {

SidebarWidget::SidebarWidget(WorkspaceManager &manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setFixedWidth(200);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list);

    auto *addButton = new QPushButton("+", this);
    addButton->setToolTip("New Workspace");
    addButton->setFixedHeight(28);
    layout->addWidget(addButton);

    connect(addButton, &QPushButton::clicked, this, [this]() {
        m_manager.addWorkspace();
    });

    connect(m_list, &QListWidget::itemClicked, this, &SidebarWidget::onItemClicked);

    connect(&m_manager, &WorkspaceManager::workspaceAdded,
            this, &SidebarWidget::onWorkspaceAdded);
    connect(&m_manager, &WorkspaceManager::workspaceRemoved,
            this, &SidebarWidget::onWorkspaceRemoved);
    connect(&m_manager, &WorkspaceManager::selectedWorkspaceChanged,
            this, &SidebarWidget::onSelectionChanged);

    // Populate existing workspaces
    for (int i = 0; i < m_manager.count(); ++i) {
        onWorkspaceAdded(m_manager.workspace(i), i);
    }
    onSelectionChanged(m_manager.selectedWorkspaceId());
}

void SidebarWidget::setVisible(bool visible)
{
    m_visible = visible;
    QWidget::setVisible(visible);
}

void SidebarWidget::toggleVisibility()
{
    setVisible(!m_visible);
}

void SidebarWidget::onWorkspaceAdded(Workspace *workspace, int index)
{
    auto *item = new QListWidgetItem();
    item->setData(Qt::UserRole, workspace->id().toString());
    m_list->insertItem(index, item);
    updateItem(index, workspace);

    connect(workspace, &Workspace::titleChanged, this, [this, workspace]() {
        int idx = m_manager.indexOf(workspace->id());
        if (idx >= 0) updateItem(idx, workspace);
    });
}

void SidebarWidget::onWorkspaceRemoved(const QUuid &id, int index)
{
    Q_UNUSED(id);
    delete m_list->takeItem(index);
}

void SidebarWidget::onSelectionChanged(const QUuid &id)
{
    m_updatingSelection = true;
    for (int i = 0; i < m_list->count(); ++i) {
        auto *item = m_list->item(i);
        bool selected = QUuid::fromString(item->data(Qt::UserRole).toString()) == id;
        item->setSelected(selected);
        if (selected) m_list->setCurrentRow(i);
    }
    m_updatingSelection = false;
}

void SidebarWidget::onItemClicked(QListWidgetItem *item)
{
    if (m_updatingSelection) return;
    auto id = QUuid::fromString(item->data(Qt::UserRole).toString());
    m_manager.selectWorkspace(id);
}

void SidebarWidget::updateItem(int index, Workspace *workspace)
{
    auto *item = m_list->item(index);
    if (item) {
        item->setText(workspace->effectiveTitle());
    }
}

} // namespace c11
