#include "TerminalPanel.h"
#include "platform/PlatformAbstraction.h"

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
    // Inject c11's shell-integration env so an agent (or the CLI) running inside
    // this pane can self-locate and reach the socket without the operator.
    const QList<QPair<QString, QString>> envVars = {
        {"C11_SURFACE_ID", id().toString(QUuid::WithoutBraces)},
        {"C11_WORKSPACE_ID", m_workspaceId.toString(QUuid::WithoutBraces)},
        {"C11_SHELL_INTEGRATION", "1"},
        {"C11_SOCKET", platform::socketPath()},
    };
    m_widget->createSurface(workingDirectory, command, envVars);
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
