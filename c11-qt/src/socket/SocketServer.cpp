#include "SocketServer.h"

#include <QFile>
#include <QDebug>

namespace c11 {

SocketServer::SocketServer(QObject *parent)
    : QObject(parent)
{
}

SocketServer::~SocketServer()
{
    stop();
}

bool SocketServer::start(const QString &socketPath)
{
    if (m_server) stop();

    m_socketPath = socketPath;

    // Remove stale socket file
    QFile::remove(socketPath);

    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);

    connect(m_server, &QLocalServer::newConnection,
            this, &SocketServer::onNewConnection);

    if (!m_server->listen(socketPath)) {
        qCritical() << "SocketServer: failed to listen on" << socketPath
                     << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    qDebug() << "SocketServer: listening on" << socketPath;
    return true;
}

void SocketServer::stop()
{
    if (m_server) {
        m_server->close();

        for (auto *client : m_clients) {
            client->disconnectFromServer();
            client->deleteLater();
        }
        m_clients.clear();

        QFile::remove(m_socketPath);

        delete m_server;
        m_server = nullptr;
    }
}

bool SocketServer::isListening() const
{
    return m_server && m_server->isListening();
}

void SocketServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        auto *client = m_server->nextPendingConnection();
        m_clients.append(client);

        connect(client, &QLocalSocket::readyRead,
                this, &SocketServer::onClientReadyRead);
        connect(client, &QLocalSocket::disconnected,
                this, &SocketServer::onClientDisconnected);

        emit clientConnected();
    }
}

void SocketServer::onClientReadyRead()
{
    auto *client = qobject_cast<QLocalSocket *>(sender());
    if (!client) return;

    while (client->canReadLine()) {
        QByteArray line = client->readLine().trimmed();
        if (line.isEmpty()) continue;

        QString command = QString::fromUtf8(line);
        emit commandReceived(command);

        if (m_handler) {
            // Process command. Catch any exceptions from command execution
            // (e.g., Ghostty surface creation failures).
            QString response;
            try {
                response = m_handler(client, command);
            } catch (...) {
                response = "ERROR: Internal error processing command\n";
            }
            if (!response.isEmpty()) {
                if (client->state() == QLocalSocket::ConnectedState) {
                    client->write(response.toUtf8());
                    if (!response.endsWith('\n')) {
                        client->write("\n");
                    }
                    client->flush();
                }
            }
        }
    }
}

void SocketServer::onClientDisconnected()
{
    auto *client = qobject_cast<QLocalSocket *>(sender());
    if (!client) return;

    m_clients.removeOne(client);
    client->deleteLater();
    emit clientDisconnected();
}

} // namespace c11
