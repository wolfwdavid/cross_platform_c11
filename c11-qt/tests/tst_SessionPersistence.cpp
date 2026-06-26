#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include "session/SessionPersistence.h"
#include "workspace/WorkspaceManager.h"
#include "workspace/Workspace.h"
#include "workspace/PaneLayout.h"
#include "panel/MarkdownPanel.h"
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

    // A split layout (multiple panes) must survive a save -> restore round trip,
    // with the panels recreated and the tree (direction + leaves) rebuilt. This
    // is the core of the session-restore fix: restoreFromSnapshot used to recreate
    // only workspace titles, dropping all panes.
    void testRoundTripSplitLayout()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("Split");
        ws->splitPanel(ws->focusedPanelId(), PaneLayout::Direction::Horizontal);
        QCOMPARE(ws->panelCount(), 2);
        QVERIFY(ws->layout()->isSplit());

        SessionPersistence sp(mgr);
        const QJsonObject snap = sp.createSnapshot();
        QVERIFY(sp.restoreFromSnapshot(snap));

        QCOMPARE(mgr.count(), 1);
        auto *r = mgr.workspace(0);
        QCOMPARE(r->panelCount(), 2);
        QVERIFY(r->layout()->isSplit());
        QCOMPARE(r->layout()->split().direction, PaneLayout::Direction::Horizontal);
        // Both leaves must resolve to panels that actually exist in the workspace.
        const auto ids = r->layout()->allPanelIds();
        QCOMPARE(ids.size(), size_t(2));
        for (const auto &id : ids) QVERIFY(r->panel(id) != nullptr);
    }

    // A markdown panel's file path is part of its state and must round-trip so the
    // restored panel reopens the same document.
    void testRoundTripMarkdownPath()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("Docs");
        auto *md = ws->createMarkdownPanel("C:/notes/readme.md");
        ws->layout()->splitLeaf(ws->focusedPanelId(), md->id(),
                                PaneLayout::Direction::Horizontal);

        SessionPersistence sp(mgr);
        const QJsonObject snap = sp.createSnapshot();
        QVERIFY(sp.restoreFromSnapshot(snap));

        auto *r = mgr.workspace(0);
        MarkdownPanel *restored = nullptr;
        for (auto *p : r->allPanels()) {
            if (auto *m = qobject_cast<MarkdownPanel *>(p)) { restored = m; break; }
        }
        QVERIFY(restored != nullptr);
        QCOMPARE(restored->filePath(), QString("C:/notes/readme.md"));
    }
};

QTEST_MAIN(TestSessionPersistence)
#include "tst_SessionPersistence.moc"
