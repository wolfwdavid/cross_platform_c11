#pragma once

#include "ghostty/GhosttyRuntime.h"
#include "ghostty/GhosttyConfig.h"
#include "socket/SocketServer.h"
#include "socket/SocketCommandRouter.h"

#include <QObject>
#include <memory>

namespace c11 {

class WorkspaceManager;

class C11Application : public QObject {
    Q_OBJECT

public:
    explicit C11Application(QObject *parent = nullptr);
    ~C11Application() override;

    bool initialize();

    GhosttyRuntime &ghosttyRuntime() { return *m_ghosttyRuntime; }
    const GhosttyConfig &ghosttyConfig() const { return m_config; }

    void reloadConfig();

    // Socket — started after WorkspaceManager is created
    void startSocketServer(WorkspaceManager &manager);
    SocketServer *socketServer() const { return m_socketServer.get(); }

signals:
    void configReloaded();

private:
    void mirrorEnvVars();

    std::unique_ptr<GhosttyRuntime> m_ghosttyRuntime;
    GhosttyConfig m_config;
    std::unique_ptr<SocketServer> m_socketServer;
    std::unique_ptr<SocketCommandRouter> m_commandRouter;
};

} // namespace c11
