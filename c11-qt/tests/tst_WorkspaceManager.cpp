#include <QTest>
#include <QSignalSpy>
#include <algorithm>
#include "workspace/WorkspaceManager.h"
#include "ghostty/GhosttyRuntime.h"

using namespace c11;

class TestWorkspaceManager : public QObject {
    Q_OBJECT

private:
    GhosttyRuntime *m_runtime = nullptr;

private slots:
    void initTestCase()
    {
        m_runtime = new GhosttyRuntime(this);
        // Stub mode — no Ghostty init needed for model tests
    }

    void testAddWorkspace()
    {
        WorkspaceManager mgr(*m_runtime);
        QSignalSpy spy(&mgr, &WorkspaceManager::workspaceAdded);

        auto *ws = mgr.addWorkspace("Test");
        QVERIFY(ws != nullptr);
        QCOMPARE(mgr.count(), 1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(ws->effectiveTitle(), "Test");
    }

    void testAutoSelectFirst()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace();
        QCOMPARE(mgr.selectedWorkspaceId(), ws->id());
    }

    void testSelectWorkspace()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws1 = mgr.addWorkspace("One");
        auto *ws2 = mgr.addWorkspace("Two");

        QSignalSpy spy(&mgr, &WorkspaceManager::selectedWorkspaceChanged);

        mgr.selectWorkspace(ws2->id());
        QCOMPARE(mgr.selectedWorkspaceId(), ws2->id());
        QCOMPARE(mgr.selectedWorkspace(), ws2);
        QCOMPARE(spy.count(), 1);

        Q_UNUSED(ws1);
    }

    void testRemoveWorkspace()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws1 = mgr.addWorkspace("One");
        mgr.addWorkspace("Two");

        mgr.selectWorkspace(ws1->id());
        mgr.removeWorkspace(ws1->id());

        QCOMPARE(mgr.count(), 1);
        QVERIFY(!mgr.selectedWorkspaceId().isNull());
    }

    void testNextPrevious()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws1 = mgr.addWorkspace("One");
        auto *ws2 = mgr.addWorkspace("Two");
        auto *ws3 = mgr.addWorkspace("Three");

        mgr.selectWorkspace(ws1->id());

        mgr.selectNextWorkspace();
        QCOMPARE(mgr.selectedWorkspaceId(), ws2->id());

        mgr.selectNextWorkspace();
        QCOMPARE(mgr.selectedWorkspaceId(), ws3->id());

        mgr.selectNextWorkspace(); // wraps
        QCOMPARE(mgr.selectedWorkspaceId(), ws1->id());

        mgr.selectPreviousWorkspace(); // wraps back
        QCOMPARE(mgr.selectedWorkspaceId(), ws3->id());
    }

    void testMoveWorkspace()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws1 = mgr.addWorkspace("One");
        auto *ws2 = mgr.addWorkspace("Two");
        auto *ws3 = mgr.addWorkspace("Three");

        mgr.moveWorkspace(0, 2);
        QCOMPARE(mgr.workspace(0), ws2);
        QCOMPARE(mgr.workspace(1), ws3);
        QCOMPARE(mgr.workspace(2), ws1);
    }

    // addPane must splice every new pane into the visible layout tree, not just
    // create it. Regression for the new-pane orphan bug: createTerminalPanel
    // alone leaves panes #2+ out of the tree (live shell, never rendered).
    void testAddPanePlacesInLayout()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");          // seeds one initial pane
        const int before = ws->panelCount();
        QCOMPARE(int(ws->orderedPanelIds().size()), before);

        auto *p = ws->addPane();
        QCOMPARE(ws->panelCount(), before + 1);
        // The new pane is in the layout tree (orderedPanelIds reads the tree, so
        // an orphaned pane would NOT appear) and is focused.
        QCOMPARE(int(ws->orderedPanelIds().size()), before + 1);
        QCOMPARE(ws->focusedPanelId(), p->id());
        const auto ids = ws->orderedPanelIds();
        QVERIFY(std::find(ids.begin(), ids.end(), p->id()) != ids.end());

        // A third pane also lands in the tree.
        auto *p3 = ws->addPane();
        QCOMPARE(int(ws->orderedPanelIds().size()), before + 2);
        QCOMPARE(ws->focusedPanelId(), p3->id());
    }

    void testIndexOf()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws1 = mgr.addWorkspace("One");
        auto *ws2 = mgr.addWorkspace("Two");

        QCOMPARE(mgr.indexOf(ws1->id()), 0);
        QCOMPARE(mgr.indexOf(ws2->id()), 1);
        QCOMPARE(mgr.indexOf(QUuid::createUuid()), -1);
    }
};

QTEST_MAIN(TestWorkspaceManager)
#include "tst_WorkspaceManager.moc"
