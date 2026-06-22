#include <QTest>
#include <QLabel>

#include "ui/StatusBar.h"
#include "workspace/WorkspaceManager.h"
#include "ghostty/GhosttyRuntime.h"

using namespace c11;

// Behavioral tests for the status bar's pane counter. The counter reads
// Workspace::panelCount() and must refresh whenever panels are added/removed,
// including on the workspace that is already selected when the bar is built.
class TestStatusBar : public QObject {
    Q_OBJECT

    GhosttyRuntime *m_runtime = nullptr;

    // The bar holds three QLabels; the pane counter is the one that mentions
    // "pane". Read it by content so we don't depend on private members.
    static QString paneText(const StatusBar &bar)
    {
        const auto labels = bar.findChildren<QLabel *>();
        for (auto *l : labels) {
            if (l->text().contains("pane")) return l->text();
        }
        return {};
    }

    static QString expected(int n)
    {
        return QString("%1 pane%2").arg(n).arg(n == 1 ? "" : "s");
    }

private slots:
    void initTestCase()
    {
        m_runtime = new GhosttyRuntime(this); // stub mode — no Ghostty init
    }

    // Regression for the stale-counter bug: the bar must track the workspace
    // that is ALREADY selected at construction. Previously panelAdded was only
    // connected inside onSelectionChanged (fires on a *change*), so the initial
    // workspace was never subscribed and the count went stale after the first
    // split.
    void paneCounterTracksInitialWorkspace()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS"); // becomes the selected workspace
        const int before = ws->panelCount();

        StatusBar bar(mgr); // subscribes to the selected workspace here
        QCOMPARE(paneText(bar), expected(before));

        ws->createTerminalPanel(); // emits panelAdded
        QCOMPARE(ws->panelCount(), before + 1);
        QCOMPARE(paneText(bar), expected(before + 1));

        ws->createTerminalPanel();
        QCOMPARE(paneText(bar), expected(before + 2));
    }

    // The counter must also follow a selection change and then track panel
    // adds on the newly selected workspace.
    void paneCounterFollowsSelection()
    {
        WorkspaceManager mgr(*m_runtime);
        mgr.addWorkspace("One");
        auto *ws2 = mgr.addWorkspace("Two");

        StatusBar bar(mgr);
        mgr.selectWorkspace(ws2->id());

        const int before = ws2->panelCount();
        ws2->createTerminalPanel();
        QCOMPARE(paneText(bar), expected(before + 1));
    }

    // Removing a panel must decrement the displayed count.
    void paneCounterTracksRemoval()
    {
        WorkspaceManager mgr(*m_runtime);
        auto *ws = mgr.addWorkspace("WS");
        auto *p = ws->createTerminalPanel();
        const int withExtra = ws->panelCount();

        StatusBar bar(mgr);
        QCOMPARE(paneText(bar), expected(withExtra));

        ws->removePanel(p->id());
        QCOMPARE(paneText(bar), expected(withExtra - 1));
    }
};

QTEST_MAIN(TestStatusBar)
#include "tst_StatusBar.moc"
