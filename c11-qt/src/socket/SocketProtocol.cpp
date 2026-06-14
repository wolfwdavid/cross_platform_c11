#include "SocketProtocol.h"

#include <QJsonArray>

namespace c11 {

SocketProtocol::Version SocketProtocol::detectVersion(const QString &line)
{
    QString trimmed = line.trimmed();
    if (trimmed.startsWith('{')) {
        return Version::V2;
    }
    return Version::V1;
}

SocketProtocol::V1Command SocketProtocol::parseV1(const QString &line)
{
    V1Command cmd;
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) return cmd;

    // Split respecting quoted strings
    QStringList parts;
    QString current;
    bool inQuotes = false;
    QChar quoteChar;

    for (int i = 0; i < trimmed.size(); ++i) {
        QChar c = trimmed[i];
        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
            } else {
                current += c;
            }
        } else if (c == '"' || c == '\'') {
            inQuotes = true;
            quoteChar = c;
        } else if (c == ' ' || c == '\t') {
            if (!current.isEmpty()) {
                parts.append(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.isEmpty()) parts.append(current);

    if (!parts.isEmpty()) {
        cmd.name = parts.takeFirst().toLower();
        cmd.args = parts;
    }

    return cmd;
}

std::optional<SocketProtocol::V2Request> SocketProtocol::parseV2(const QString &line)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    QJsonObject obj = doc.object();
    V2Request req;
    req.id = obj.value("id");
    req.method = obj.value("method").toString();
    req.params = obj.value("params").toObject();

    if (req.method.isEmpty()) {
        return std::nullopt;
    }

    return req;
}

QString SocketProtocol::v1Ok(const QString &body)
{
    if (body.isEmpty()) return QStringLiteral("OK\n");
    return body + "\n";
}

QString SocketProtocol::v1Error(const QString &message)
{
    return "ERROR: " + message + "\n";
}

QString SocketProtocol::v2Ok(const QJsonValue &id, const QJsonValue &result)
{
    QJsonObject obj;
    obj["ok"] = true;
    obj["id"] = id;
    if (!result.isNull() && !result.isUndefined()) {
        obj["result"] = result;
    }
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)) + "\n";
}

QString SocketProtocol::v2Error(const QJsonValue &id, const QString &code, const QString &message)
{
    QJsonObject err;
    err["code"] = code;
    err["message"] = message;

    QJsonObject obj;
    obj["ok"] = false;
    obj["id"] = id;
    obj["error"] = err;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)) + "\n";
}

QString SocketProtocol::v1Arg(const QStringList &args, const QString &key, const QString &defaultValue)
{
    QString prefix = "--" + key + "=";
    for (const auto &arg : args) {
        if (arg.startsWith(prefix)) {
            return arg.mid(prefix.size());
        }
    }
    return defaultValue;
}

bool SocketProtocol::v1HasFlag(const QStringList &args, const QString &flag)
{
    return args.contains("--" + flag);
}

} // namespace c11
