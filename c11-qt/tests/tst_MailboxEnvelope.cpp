#include <QTest>
#include "mailbox/MailboxEnvelope.h"

using namespace c11;

class TestMailboxEnvelope : public QObject {
    Q_OBJECT

private slots:
    void testCreateAndValidate()
    {
        MailboxEnvelope env;
        env.id = MailboxEnvelope::generateId();
        env.from = "surface-123";
        env.timestamp = 1700000000000;
        env.body = "Hello, world!";

        QVERIFY(env.isValid());
    }

    void testInvalid()
    {
        MailboxEnvelope env;
        QVERIFY(!env.isValid()); // Missing required fields
    }

    void testEncodeAndDecode()
    {
        MailboxEnvelope original;
        original.id = MailboxEnvelope::generateId();
        original.from = "surface-abc";
        original.timestamp = 1700000000000;
        original.body = "Test message";
        original.to = "surface-xyz";
        original.topic = "build";
        original.urgent = true;

        QByteArray encoded = original.encode();
        MailboxEnvelope decoded = MailboxEnvelope::decode(encoded);

        QVERIFY(decoded.isValid());
        QCOMPARE(decoded.id, original.id);
        QCOMPARE(decoded.from, original.from);
        QCOMPARE(decoded.body, original.body);
        QCOMPARE(decoded.to, original.to);
        QCOMPARE(decoded.topic, original.topic);
        QCOMPARE(decoded.urgent, true);
    }

    void testGenerateId()
    {
        QString id1 = MailboxEnvelope::generateId();
        QString id2 = MailboxEnvelope::generateId();
        QVERIFY(!id1.isEmpty());
        QVERIFY(!id2.isEmpty());
        QVERIFY(id1 != id2); // Should be unique
    }

    void testJsonRoundtrip()
    {
        MailboxEnvelope env;
        env.id = "test-id-123";
        env.from = "sender";
        env.timestamp = 1234567890;
        env.body = "payload";
        env.ttlSeconds = 60;

        QJsonObject json = env.toJson();
        MailboxEnvelope restored = MailboxEnvelope::fromJson(json);

        QCOMPARE(restored.id, env.id);
        QCOMPARE(restored.from, env.from);
        QCOMPARE(restored.timestamp, env.timestamp);
        QCOMPARE(restored.body, env.body);
        QCOMPARE(restored.ttlSeconds, 60);
    }
};

QTEST_GUILESS_MAIN(TestMailboxEnvelope)
#include "tst_MailboxEnvelope.moc"
