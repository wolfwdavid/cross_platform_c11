#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>

#include "socket/SocketCommandRouter.h"
#include "workspace/WorkspaceManager.h"
#include "panel/TerminalPanel.h"
#include "ghostty/GhosttyRuntime.h"

using namespace c11;

// Behavioral tests for the `surface.send` route and its shared targeting seam
// (resolvePanel). Byte delivery itself (ghostty_surface_text) is a no-op in
// C11_GHOSTTY_STUB builds, so these exercise routing, panel resolution, and the
// success/error envelopes — the parts that have observable behavior without a
// real libghostty.
class TestSocketCommandRouter : public QObject {
    Q_OBJECT

    GhosttyRuntime *m_runtime = nullptr;

    // Parse a JSON-RPC response line and return its `result` object.
    static QJsonObject resultOf(const QString &response)
    {
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
        return doc.object().value("result").toObject();
    }

    static QString sendLine(const QString &id, const QString &text, bool submit = true)
    {
        QJsonObject params;
        if (!id.isEmpty()) params["id"] = id;
        params["text"] = text;
        params["submit"] = submit;
        QJsonObject req;
        req["id"] = 1;
        req["method"] = "surface.send";
        req["params"] = params;
        return QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
    }

private slots:
    void initTestCase()
    {
        m_runtime = new GhosttyRuntime(this); // stub mode
    }

    // Explicit surface ref → resolves and reports the same surface id back.
    void sendToExplicitSurface()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();
        const QString pid = panel->id().toString(QUuid::WithoutBraces);

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(sendLine(pid, "npm test")));

        QVERIFY(result.value("ok").toBool());
        QCOMPARE(result.value("surface").toString(), pid);
        QCOMPARE(result.value("sent").toInt(), int(QString("npm test").size()));
        QVERIFY(result.value("submitted").toBool());
    }

    // No ref → falls back to the focused panel of the selected workspace.
    void sendFallsBackToFocused()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();
        ws->setFocusedPanelId(panel->id());

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(sendLine(QString(), "ls")));

        QVERIFY(result.value("ok").toBool());
        QCOMPARE(result.value("surface").toString(),
                 panel->id().toString(QUuid::WithoutBraces));
    }

    // --no-submit is honored (no synthetic Return).
    void sendRespectsNoSubmit()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(
            sendLine(panel->id().toString(QUuid::WithoutBraces), "cd /tmp/", false)));

        QVERIFY(result.value("ok").toBool());
        QVERIFY(!result.value("submitted").toBool());
    }

    // An unknown UUID resolves to nothing → not_found.
    void sendToMissingSurfaceErrors()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("WS");

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(
            sendLine(QUuid::createUuid().toString(QUuid::WithoutBraces), "x")));

        QCOMPARE(result.value("error").toString(), QString("not_found"));
    }

    // Missing the required `text` param → missing_text, never a partial send.
    void sendWithoutTextErrors()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        ws->createTerminalPanel();

        SocketCommandRouter router(mgr);
        QJsonObject req;
        req["id"] = 1;
        req["method"] = "surface.send";
        req["params"] = QJsonObject{}; // no text
        auto result = resultOf(router.processLine(
            QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact))));

        QCOMPARE(result.value("error").toString(), QString("missing_text"));
    }

    // --- surface.send_key ---

    static QString sendKeyLine(const QString &id, const QString &key)
    {
        QJsonObject params;
        if (!id.isEmpty()) params["id"] = id;
        params["key"] = key;
        QJsonObject req;
        req["id"] = 1;
        req["method"] = "surface.send_key";
        req["params"] = params;
        return QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
    }

    void sendKeyToTerminal()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();
        const QString pid = panel->id().toString(QUuid::WithoutBraces);

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(sendKeyLine(pid, "ctrl+c")));

        QVERIFY(result.value("ok").toBool());
        QCOMPARE(result.value("surface").toString(), pid);
        QCOMPARE(result.value("key").toString(), QString("ctrl+c"));
    }

    void sendKeyWithoutKeyErrors()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        ws->createTerminalPanel();

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(sendKeyLine(QString(), "")));
        QCOMPARE(result.value("error").toString(), QString("missing_key"));
    }

    void sendKeyRejectsBadChord()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(
            sendKeyLine(panel->id().toString(QUuid::WithoutBraces), "notakey")));
        QCOMPARE(result.value("error").toString(), QString("bad_key"));
    }

    void sendKeyToMissingSurfaceErrors()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("WS");

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(
            sendKeyLine(QUuid::createUuid().toString(QUuid::WithoutBraces), "enter")));
        QCOMPARE(result.value("error").toString(), QString("not_found"));
    }

    // --- surface.read_screen ---

    static QString readScreenLine(const QString &id)
    {
        QJsonObject params;
        if (!id.isEmpty()) params["id"] = id;
        QJsonObject req;
        req["id"] = 1;
        req["method"] = "surface.read_screen";
        req["params"] = params;
        return QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
    }

    void readScreenReturnsTextField()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();
        const QString pid = panel->id().toString(QUuid::WithoutBraces);

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(readScreenLine(pid)));

        QVERIFY(result.value("ok").toBool());
        QCOMPARE(result.value("surface").toString(), pid);
        // Stub build: no live surface text, but the field is always present.
        QVERIFY(result.contains("text"));
    }

    void readScreenMissingSurfaceErrors()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("WS");

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(
            readScreenLine(QUuid::createUuid().toString(QUuid::WithoutBraces))));
        QCOMPARE(result.value("error").toString(), QString("not_found"));
    }

    // --- system.identify ---

    static QString identifyLine(const QString &surfaceId)
    {
        QJsonObject params;
        if (!surfaceId.isEmpty()) params["surface"] = surfaceId;
        QJsonObject req;
        req["id"] = 1;
        req["method"] = "system.identify";
        req["params"] = params;
        return QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
    }

    void identifyResolvesCaller()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *panel = ws->createTerminalPanel();
        const QString pid = panel->id().toString(QUuid::WithoutBraces);
        const QString wid = ws->id().toString(QUuid::WithoutBraces);

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(identifyLine(pid)));

        QVERIFY(result.value("has_caller").toBool());
        const QJsonObject caller = result.value("caller").toObject();
        QCOMPARE(caller.value("surface_ref").toString(), pid);
        QCOMPARE(caller.value("pane_ref").toString(), pid);
        QCOMPARE(caller.value("workspace_ref").toString(), wid);
    }

    void identifyUnknownCallerIsEmpty()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("WS");

        SocketCommandRouter router(mgr);
        auto result = resultOf(router.processLine(
            identifyLine(QUuid::createUuid().toString(QUuid::WithoutBraces))));
        QVERIFY(!result.value("has_caller").toBool());
    }
};

QTEST_MAIN(TestSocketCommandRouter)
#include "tst_SocketCommandRouter.moc"
