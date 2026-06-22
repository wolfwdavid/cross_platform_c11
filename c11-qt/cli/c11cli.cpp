#include <QCoreApplication>
#include <QLocalSocket>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>

static QString defaultSocketPath()
{
    QByteArray envPath = qgetenv("C11_SOCKET");
    if (envPath.isEmpty()) envPath = qgetenv("CMUX_SOCKET");
    if (!envPath.isEmpty()) return QString::fromUtf8(envPath);

#ifdef Q_OS_MACOS
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/c11/c11.sock";
#elif defined(Q_OS_WIN)
    return "\\\\.\\pipe\\c11-" + QString::fromLocal8Bit(qgetenv("USERNAME"));
#else
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) runtimeDir = "/tmp";
    return runtimeDir + "/c11.sock";
#endif
}

static QString sendCommand(const QString &socketPath, const QString &command, int timeoutMs = 5000)
{
    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(timeoutMs)) {
        return "ERROR: Cannot connect to c11 at " + socketPath + ": " + socket.errorString();
    }

    QByteArray data = command.toUtf8();
    if (!data.endsWith('\n')) data.append('\n');
    socket.write(data);
    socket.flush();

    if (!socket.waitForReadyRead(timeoutMs)) {
        socket.disconnectFromServer();
        return "ERROR: Timeout waiting for response";
    }

    QByteArray response;
    while (socket.bytesAvailable() > 0 || socket.waitForReadyRead(500)) {
        response.append(socket.readAll());
        if (response.endsWith('\n')) break;
    }

    socket.disconnectFromServer();
    return QString::fromUtf8(response).trimmed();
}

static QString buildV2Request(const QString &method, const QJsonObject &params = {})
{
    QJsonObject req;
    req["id"] = 1;
    req["method"] = method;
    if (!params.isEmpty()) req["params"] = params;
    return QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    // Manual arg parsing to avoid QCommandLineParser eating --key=value args
    QStringList allArgs;
    for (int i = 1; i < argc; ++i) allArgs << QString::fromUtf8(argv[i]);

    // Extract our own flags first
    QString socketPath;
    bool useJson = false;
    QStringList remaining;

    for (int i = 0; i < allArgs.size(); ++i) {
        const auto &arg = allArgs[i];
        if (arg == "--socket" && i + 1 < allArgs.size()) {
            socketPath = allArgs[++i];
        } else if (arg.startsWith("--socket=")) {
            socketPath = arg.mid(9);
        } else if (arg == "--json") {
            useJson = true;
        } else if (arg == "--help" || arg == "-h") {
            out << "Usage: c11 [--socket=PATH] [--json] COMMAND [ARGS...]\n\n"
                << "Commands:\n"
                << "  ping                 Test connection\n"
                << "  tree                 Show workspace tree (V2)\n"
                << "  list-workspaces      List all workspaces\n"
                << "  new-workspace        Create workspace\n"
                << "  close-workspace      Close workspace\n"
                << "  select-workspace     Select workspace by id/index\n"
                << "  next-workspace       Switch to next workspace\n"
                << "  prev-workspace       Switch to previous workspace\n"
                << "  list-surfaces        List all surfaces/panels\n"
                << "  new-pane             Create new terminal pane\n"
                << "  new-split            Split current pane\n"
                << "  close-surface        Close surface/panel\n"
                << "  open-browser         Open browser panel\n"
                << "  capabilities         Show capabilities (V2)\n"
                << "\nOptions:\n"
                << "  --socket=PATH        Socket path (default: auto-detect)\n"
                << "  --json               Force JSON-RPC (V2) protocol\n";
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            out << "c11 " << C11_VERSION << "\n";
            return 0;
        } else {
            remaining << arg;
        }
    }

    if (socketPath.isEmpty()) socketPath = defaultSocketPath();

    if (remaining.isEmpty()) {
        err << "Error: no command specified. Use --help for usage.\n";
        return 1;
    }

    QString command = remaining.takeFirst();

    // V2 method map
    static const QMap<QString, QString> v2Methods = {
        {"ping",              "system.ping"},
        {"tree",              "system.tree"},
        {"capabilities",      "system.capabilities"},
        {"list-workspaces",   "workspace.list"},
        {"current-workspace", "workspace.current"},
        {"new-workspace",     "workspace.create"},
        {"close-workspace",   "workspace.close"},
        {"select-workspace",  "workspace.select"},
        {"next-workspace",    "workspace.next"},
        {"prev-workspace",    "workspace.previous"},
        {"list-surfaces",     "surface.list"},
        {"new-pane",          "surface.create"},
        {"new-split",         "surface.split"},
        {"close-surface",     "surface.close"},
        {"list-panes",        "pane.list"},
        {"open-browser",      "browser.open_split"},
    };

    // V1 alias map
    static const QMap<QString, QString> v1Aliases = {
        {"list-workspaces",   "list_workspaces"},
        {"new-workspace",     "new_workspace"},
        {"close-workspace",   "close_workspace"},
        {"select-workspace",  "select_workspace"},
        {"current-workspace", "current_workspace"},
        {"list-surfaces",     "list_surfaces"},
        {"new-pane",          "new_pane"},
        {"new-split",         "new_split"},
        {"close-surface",     "close_surface"},
        {"set-status",        "set_status"},
        {"clear-status",      "clear_status"},
        {"open-browser",      "open_browser"},
    };

    if (useJson || v2Methods.contains(command)) {
        // V2 JSON-RPC
        QString method = v2Methods.value(command, command);
        QJsonObject params;

        // Store a value under the right JSON type. The router reads most params
        // as strings, but `index` as an int and `select` as a bool, so a bare
        // string would be silently coerced to 0 / the default.
        auto setParam = [&](const QString &key, const QString &value) {
            if (key == "index") {
                params[key] = value.toInt();
            } else if (key == "select") {
                params[key] = (value == "true" || value == "1");
            } else {
                params[key] = value;
            }
        };

        QStringList positionals;
        for (const auto &arg : remaining) {
            int eq = arg.indexOf('=');
            if (eq > 0) { // key=value or --key=value
                QString key = arg.left(eq);
                if (key.startsWith("--")) key = key.mid(2);
                setParam(key, arg.mid(eq + 1));
            } else if (!arg.startsWith("-")) {
                positionals << arg; // bare value, mapped per-command below
            }
        }

        // Map leading positional args to the param each command expects, so the
        // natural CLI form works (e.g. `open-browser <url>`, `select-workspace 2`)
        // and not only the explicit `key=value` form.
        if (!positionals.isEmpty()) {
            const QString &first = positionals.first();
            if (command == "select-workspace") {
                // A number is a 0-based index; anything else is a workspace id.
                bool isInt = false;
                first.toInt(&isInt);
                setParam(isInt ? "index" : "id", first);
            } else {
                static const QMap<QString, QString> firstPositional = {
                    {"open-browser",    "url"},
                    {"new-split",       "direction"},
                    {"new-workspace",   "title"},
                    {"new-pane",        "cwd"},
                    {"close-workspace", "id"},
                    {"close-surface",   "id"},
                };
                const QString key = firstPositional.value(command);
                if (!key.isEmpty() && !params.contains(key)) setParam(key, first);
            }
        }

        QString request = buildV2Request(method, params);
        QString response = sendCommand(socketPath, request);

        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
        if (!doc.isNull()) {
            out << doc.toJson(QJsonDocument::Indented);
        } else {
            out << response << "\n";
        }
    } else {
        // V1 text — pass remaining args through verbatim
        QString v1Name = v1Aliases.value(command, command);
        QString fullCommand = v1Name;
        if (!remaining.isEmpty()) {
            fullCommand += " " + remaining.join(" ");
        }

        QString response = sendCommand(socketPath, fullCommand);
        out << response << "\n";
    }

    return 0;
}
