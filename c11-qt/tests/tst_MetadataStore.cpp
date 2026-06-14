#include <QTest>
#include <QJsonObject>
#include "metadata/SurfaceMetadataStore.h"

using namespace c11;

class TestMetadataStore : public QObject {
    Q_OBJECT

private slots:
    void testSetAndGet()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        QJsonObject data;
        data["status"] = "running";
        data["task"] = "building";

        auto result = store.setMetadata(id, data);
        QVERIFY(result.ok);

        QJsonObject got = store.getMetadata(id);
        QCOMPARE(got["status"].toString(), "running");
        QCOMPARE(got["task"].toString(), "building");

        store.removeSurface(id);
    }

    void testMergeMode()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        QJsonObject first;
        first["status"] = "idle";
        store.setMetadata(id, first);

        QJsonObject second;
        second["task"] = "compiling";
        store.setMetadata(id, second, SurfaceMetadataStore::Source::Explicit,
                          SurfaceMetadataStore::WriteMode::Merge);

        QJsonObject got = store.getMetadata(id);
        QCOMPARE(got["status"].toString(), "idle");
        QCOMPARE(got["task"].toString(), "compiling");

        store.removeSurface(id);
    }

    void testPrecedence()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        // Set heuristic (low precedence)
        QJsonObject heuristic;
        heuristic["terminal_type"] = "guessed";
        store.setMetadata(id, heuristic, SurfaceMetadataStore::Source::Heuristic);

        // Set explicit (high precedence)
        QJsonObject explicit_;
        explicit_["terminal_type"] = "claude-code";
        store.setMetadata(id, explicit_, SurfaceMetadataStore::Source::Explicit);

        QJsonObject got = store.getMetadata(id);
        QCOMPARE(got["terminal_type"].toString(), "claude-code");

        store.removeSurface(id);
    }

    void testClearKey()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        QJsonObject data;
        data["status"] = "done";
        data["task"] = "test";
        store.setMetadata(id, data);

        store.clearMetadata(id, "status");

        QJsonObject got = store.getMetadata(id);
        QVERIFY(!got.contains("status"));
        QCOMPARE(got["task"].toString(), "test");

        store.removeSurface(id);
    }

    void testClearAll()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        QJsonObject data;
        data["status"] = "running";
        store.setMetadata(id, data);

        store.clearMetadata(id);
        QJsonObject got = store.getMetadata(id);
        QVERIFY(got.isEmpty());

        store.removeSurface(id);
    }

    void testPayloadCap()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        // Create a payload larger than 64KB
        QString big(70000, 'x');
        QJsonObject data;
        data["huge"] = big;

        auto result = store.setMetadata(id, data);
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains("64KB"));

        store.removeSurface(id);
    }

    void testRemoveSurface()
    {
        auto &store = SurfaceMetadataStore::instance();
        QUuid id = QUuid::createUuid();

        QJsonObject data;
        data["status"] = "active";
        store.setMetadata(id, data);

        store.removeSurface(id);
        QJsonObject got = store.getMetadata(id);
        QVERIFY(got.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestMetadataStore)
#include "tst_MetadataStore.moc"
