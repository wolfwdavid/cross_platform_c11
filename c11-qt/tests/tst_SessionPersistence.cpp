#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include "session/SessionPersistence.h"
#include "workspace/WorkspaceManager.h"
#include "ghostty/GhosttyRuntime.h"

using namespace c11;

class TestSessionPersistence : public QObject {
    Q_OBJECT

private:
    GhosttyRuntime *m_runtime = nullptr;

private slots:
    void initTestCase()
    {
        m_runtime = new GhosttyRuntime(this);
    }

    void testCreateSnapshot()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("Work");
        mgr.addWorkspace("Personal");

        SessionPersistence sp(mgr);
        QJsonObject snapshot = sp.createSnapshot();

        QCOMPARE(snapshot["schema_version"].toInt(), 1);
        QJsonArray workspaces = snapshot["workspaces"].toArray();
        QCOMPARE(workspaces.size(), 2);
        QCOMPARE(workspaces[0].toObject()["title"].toString(), "Work");
        QCOMPARE(workspaces[1].toObject()["title"].toString(), "Personal");
    }

    void testSnapshotContainsLayout()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("Test");

        SessionPersistence sp(mgr);
        QJsonObject snapshot = sp.createSnapshot();

        QJsonArray workspaces = snapshot["workspaces"].toArray();
        QJsonObject ws = workspaces[0].toObject();
        QVERIFY(ws.contains("layout"));
        QCOMPARE(ws["layout"].toObject()["type"].toString(), "leaf");
    }

    void testSnapshotSelectedIndex()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("First");
        auto *ws2 = mgr.addWorkspace("Second");
        mgr.selectWorkspace(ws2->id());

        SessionPersistence sp(mgr);
        QJsonObject snapshot = sp.createSnapshot();
        QCOMPARE(snapshot["selected_workspace_index"].toInt(), 1);
    }

    void testRestoreFromSnapshot()
    {
        // Create a snapshot
        QJsonObject snapshot;
        snapshot["schema_version"] = 1;
        snapshot["selected_workspace_index"] = 0;

        QJsonArray workspaces;
        QJsonObject ws1;
        ws1["title"] = "Restored WS";
        ws1["pinned"] = true;
        workspaces.append(ws1);
        snapshot["workspaces"] = workspaces;

        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("Original");

        SessionPersistence sp(mgr);
        bool ok = sp.restoreFromSnapshot(snapshot);
        QVERIFY(ok);
        QCOMPARE(mgr.count(), 1);
        QCOMPARE(mgr.workspace(0)->effectiveTitle(), "Restored WS");
        QVERIFY(mgr.workspace(0)->isPinned());
    }

    void testPanelTypes()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("Test");

        SessionPersistence sp(mgr);
        QJsonObject snapshot = sp.createSnapshot();

        QJsonArray panels = snapshot["workspaces"].toArray()[0]
                                .toObject()["panels"].toArray();
        QCOMPARE(panels.size(), 1);
        QCOMPARE(panels[0].toObject()["type"].toString(), "terminal");
    }
};

QTEST_MAIN(TestSessionPersistence)
#include "tst_SessionPersistence.moc"
