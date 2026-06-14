#pragma once

#include <QObject>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>
#include <QList>
#include <functional>

namespace c11 {

// Unix domain socket server (macOS/Linux) or named pipe (Windows).
// Accepts connections, reads newline-delimited commands, and dispatches
// them via a handler callback.
class SocketServer : public QObject {
    Q_OBJECT

public:
    using CommandHandler = std::function<QString(QLocalSocket *client, const QString &line)>;

    explicit SocketServer(QObject *parent = nullptr);
    ~SocketServer() override;

    bool start(const QString &socketPath);
    void stop();
    bool isListening() const;
    QString socketPath() const { return m_socketPath; }

    void setCommandHandler(CommandHandler handler) { m_handler = std::move(handler); }

signals:
    void clientConnected();
    void clientDisconnected();
    void commandReceived(const QString &command);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    QLocalServer *m_server = nullptr;
    QList<QLocalSocket *> m_clients;
    CommandHandler m_handler;
    QString m_socketPath;
};

} // namespace c11
