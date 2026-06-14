#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStringList>
#include <optional>

namespace c11 {

// Parses V1 text commands and V2 JSON-RPC requests.
struct SocketProtocol {
    enum class Version { V1, V2 };

    struct V1Command {
        QString name;        // lowercased command name
        QStringList args;    // space-separated arguments
    };

    struct V2Request {
        QJsonValue id;       // request id (string or int)
        QString method;      // dot-notation method name
        QJsonObject params;  // parameters
    };

    // Detect protocol version from a line
    static Version detectVersion(const QString &line);

    // Parse a V1 text command: "command arg1 arg2 --flag=value"
    static V1Command parseV1(const QString &line);

    // Parse a V2 JSON-RPC request
    static std::optional<V2Request> parseV2(const QString &line);

    // Format V1 response
    static QString v1Ok(const QString &body = {});
    static QString v1Error(const QString &message);

    // Format V2 response
    static QString v2Ok(const QJsonValue &id, const QJsonValue &result = QJsonValue());
    static QString v2Error(const QJsonValue &id, const QString &code, const QString &message);

    // Parse V1 arguments: handles --key=value and positional args
    static QString v1Arg(const QStringList &args, const QString &key, const QString &defaultValue = {});
    static bool v1HasFlag(const QStringList &args, const QString &flag);
};

} // namespace c11
