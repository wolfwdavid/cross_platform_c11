#include "TerminalPanel.h"

namespace c11 {

TerminalPanel::TerminalPanel(GhosttyRuntime &runtime,
                             const QUuid &workspaceId,
                             const QString &workingDirectory,
                             const QString &command,
                             QObject *parent)
    : Panel(PanelType::Terminal, parent)
    , m_widget(new GhosttyWidget(runtime))
    , m_workspaceId(workspaceId)
{
    m_widget->createSurface(workingDirectory, command);
}

TerminalPanel::~TerminalPanel()
{
    delete m_widget;
}

void TerminalPanel::focus()
{
    m_widget->setFocused(true);
    m_widget->setFocus();
}

void TerminalPanel::unfocus()
{
    m_widget->setFocused(false);
}

void TerminalPanel::close()
{
    m_widget->destroySurface();
    emit closed();
}

void TerminalPanel::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged(title);
    }
}

bool TerminalPanel::processExited() const
{
    return m_widget->processExited();
}

} // namespace c11
