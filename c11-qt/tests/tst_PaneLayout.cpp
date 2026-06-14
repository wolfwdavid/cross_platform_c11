#include <QTest>
#include "workspace/PaneLayout.h"

using namespace c11;

class TestPaneLayout : public QObject {
    Q_OBJECT

private slots:
    void testMakeLeaf()
    {
        auto id = QUuid::createUuid();
        auto layout = PaneLayout::makeLeaf(id);
        QVERIFY(layout->isLeaf());
        QCOMPARE(layout->leaf().panelId, id);
        QCOMPARE(layout->leafCount(), 1);
    }

    void testMakeSplit()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto layout = PaneLayout::makeSplit(
            PaneLayout::Direction::Horizontal,
            PaneLayout::makeLeaf(id1),
            PaneLayout::makeLeaf(id2));
        QVERIFY(layout->isSplit());
        QCOMPARE(layout->leafCount(), 2);
    }

    void testFindLeaf()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto layout = PaneLayout::makeSplit(
            PaneLayout::Direction::Horizontal,
            PaneLayout::makeLeaf(id1),
            PaneLayout::makeLeaf(id2));

        QVERIFY(layout->findLeaf(id1) != nullptr);
        QVERIFY(layout->findLeaf(id2) != nullptr);
        QVERIFY(layout->findLeaf(QUuid::createUuid()) == nullptr);
    }

    void testSplitLeaf()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto layout = PaneLayout::makeLeaf(id1);

        QVERIFY(layout->splitLeaf(id1, id2, PaneLayout::Direction::Vertical));
        QVERIFY(layout->isSplit());
        QCOMPARE(layout->leafCount(), 2);
        QVERIFY(layout->findLeaf(id1) != nullptr);
        QVERIFY(layout->findLeaf(id2) != nullptr);
    }

    void testRemovePanel()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto layout = PaneLayout::makeSplit(
            PaneLayout::Direction::Horizontal,
            PaneLayout::makeLeaf(id1),
            PaneLayout::makeLeaf(id2));

        QVERIFY(layout->removePanel(id1));
        QVERIFY(layout->isLeaf());
        QCOMPARE(layout->leaf().panelId, id2);
    }

    void testRemoveFromNestedSplit()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto id3 = QUuid::createUuid();

        auto layout = PaneLayout::makeSplit(
            PaneLayout::Direction::Horizontal,
            PaneLayout::makeLeaf(id1),
            PaneLayout::makeSplit(
                PaneLayout::Direction::Vertical,
                PaneLayout::makeLeaf(id2),
                PaneLayout::makeLeaf(id3)));

        QCOMPARE(layout->leafCount(), 3);
        QVERIFY(layout->removePanel(id2));
        QCOMPARE(layout->leafCount(), 2);
        QVERIFY(layout->findLeaf(id3) != nullptr);
    }

    void testAllPanelIds()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto id3 = QUuid::createUuid();

        auto layout = PaneLayout::makeSplit(
            PaneLayout::Direction::Horizontal,
            PaneLayout::makeLeaf(id1),
            PaneLayout::makeSplit(
                PaneLayout::Direction::Vertical,
                PaneLayout::makeLeaf(id2),
                PaneLayout::makeLeaf(id3)));

        auto ids = layout->allPanelIds();
        QCOMPARE(ids.size(), 3u);
        QCOMPARE(ids[0], id1);
        QCOMPARE(ids[1], id2);
        QCOMPARE(ids[2], id3);
    }

    void testSplitLeafInsertBefore()
    {
        auto id1 = QUuid::createUuid();
        auto id2 = QUuid::createUuid();
        auto layout = PaneLayout::makeLeaf(id1);

        QVERIFY(layout->splitLeaf(id1, id2, PaneLayout::Direction::Horizontal, false));
        auto ids = layout->allPanelIds();
        QCOMPARE(ids[0], id2); // new panel is first
        QCOMPARE(ids[1], id1);
    }

    void testSplitNonexistentLeaf()
    {
        auto id1 = QUuid::createUuid();
        auto layout = PaneLayout::makeLeaf(id1);
        QVERIFY(!layout->splitLeaf(QUuid::createUuid(), QUuid::createUuid(),
                                    PaneLayout::Direction::Horizontal));
    }

    void testRemoveNonexistent()
    {
        auto id1 = QUuid::createUuid();
        auto layout = PaneLayout::makeLeaf(id1);
        QVERIFY(!layout->removePanel(QUuid::createUuid()));
    }
};

QTEST_GUILESS_MAIN(TestPaneLayout)
#include "tst_PaneLayout.moc"
