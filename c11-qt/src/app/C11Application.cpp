#include "C11Application.h"

#include "workspace/WorkspaceManager.h"
#include "platform/PlatformAbstraction.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QProcessEnvironment>
#include <cstdlib>

namespace c11 {

C11Application::C11Application(QObject *parent)
    : QObject(parent)
{
}

C11Application::~C11Application() = default;

bool C11Application::initialize()
{
    mirrorEnvVars();

    m_config = GhosttyConfig::load();

    m_ghosttyRuntime = std::make_unique<GhosttyRuntime>();
    if (!m_ghosttyRuntime->initialize(m_config)) {
        return false;
    }

    return true;
}

void C11Application::reloadConfig()
{
    m_config = GhosttyConfig::load();
    m_ghosttyRuntime->updateConfig(m_config);
    emit configReloaded();
}

void C11Application::startSocketServer(WorkspaceManager &manager)
{
    m_commandRouter = std::make_unique<SocketCommandRouter>(manager);
    m_socketServer = std::make_unique<SocketServer>();

    m_socketServer->setCommandHandler(
        [this](QLocalSocket *, const QString &line) -> QString {
            return m_commandRouter->processLine(line);
        });

    QString sockPath = platform::socketPath();

    // Ensure parent directory exists
    QFileInfo fi(sockPath);
    QDir().mkpath(fi.absolutePath());

    if (!m_socketServer->start(sockPath)) {
        qWarning() << "Failed to start socket server at" << sockPath;
    }
}

void C11Application::mirrorEnvVars()
{
    // Mirror CMUX_* <-> C11_* env vars for backward compatibility
    auto env = QProcessEnvironment::systemEnvironment();
    for (const auto &key : env.keys()) {
        if (key.startsWith("CMUX_")) {
            auto mirror = "C11_" + key.mid(5);
            if (!env.contains(mirror)) {
                qputenv(mirror.toUtf8().constData(), env.value(key).toUtf8());
            }
        } else if (key.startsWith("C11_")) {
            auto mirror = "CMUX_" + key.mid(4);
            if (!env.contains(mirror)) {
                qputenv(mirror.toUtf8().constData(), env.value(key).toUtf8());
            }
        }
    }
}

} // namespace c11
