#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include "socket/SocketProtocol.h"

using namespace c11;

class TestSocketProtocol : public QObject {
    Q_OBJECT

private slots:
    void testDetectV1()
    {
        QCOMPARE(SocketProtocol::detectVersion("ping"), SocketProtocol::Version::V1);
        QCOMPARE(SocketProtocol::detectVersion("list_workspaces"), SocketProtocol::Version::V1);
    }

    void testDetectV2()
    {
        QCOMPARE(SocketProtocol::detectVersion("{\"method\":\"system.ping\"}"),
                 SocketProtocol::Version::V2);
    }

    void testParseV1Simple()
    {
        auto cmd = SocketProtocol::parseV1("ping");
        QCOMPARE(cmd.name, "ping");
        QVERIFY(cmd.args.isEmpty());
    }

    void testParseV1WithArgs()
    {
        auto cmd = SocketProtocol::parseV1("new_workspace My Terminal");
        QCOMPARE(cmd.name, "new_workspace");
        QCOMPARE(cmd.args.size(), 2);
        QCOMPARE(cmd.args[0], "My");
        QCOMPARE(cmd.args[1], "Terminal");
    }

    void testParseV1WithQuotedArgs()
    {
        auto cmd = SocketProtocol::parseV1("new_workspace \"My Terminal\"");
        QCOMPARE(cmd.name, "new_workspace");
        QCOMPARE(cmd.args.size(), 1);
        QCOMPARE(cmd.args[0], "My Terminal");
    }

    void testParseV1CaseInsensitive()
    {
        auto cmd = SocketProtocol::parseV1("PING");
        QCOMPARE(cmd.name, "ping");
    }

    void testParseV2()
    {
        auto req = SocketProtocol::parseV2(
            "{\"id\":1,\"method\":\"system.ping\",\"params\":{}}");
        QVERIFY(req.has_value());
        QCOMPARE(req->method, "system.ping");
        QCOMPARE(req->id.toInt(), 1);
    }

    void testParseV2Invalid()
    {
        auto req = SocketProtocol::parseV2("not json");
        QVERIFY(!req.has_value());
    }

    void testParseV2MissingMethod()
    {
        auto req = SocketProtocol::parseV2("{\"id\":1}");
        QVERIFY(!req.has_value());
    }

    void testV1Ok()
    {
        QCOMPARE(SocketProtocol::v1Ok(), "OK\n");
        QCOMPARE(SocketProtocol::v1Ok("hello"), "hello\n");
    }

    void testV1Error()
    {
        QCOMPARE(SocketProtocol::v1Error("bad"), "ERROR: bad\n");
    }

    void testV2Ok()
    {
        QString response = SocketProtocol::v2Ok(1, QJsonValue("pong"));
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object()["ok"].toBool(), true);
        QCOMPARE(doc.object()["id"].toInt(), 1);
        QCOMPARE(doc.object()["result"].toString(), "pong");
    }

    void testV2Error()
    {
        QString response = SocketProtocol::v2Error(1, "not_found", "No such method");
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object()["ok"].toBool(), false);
        auto err = doc.object()["error"].toObject();
        QCOMPARE(err["code"].toString(), "not_found");
    }

    void testV1ArgParsing()
    {
        QStringList args = {"--cwd=/tmp", "--direction=down", "extra"};
        QCOMPARE(SocketProtocol::v1Arg(args, "cwd"), "/tmp");
        QCOMPARE(SocketProtocol::v1Arg(args, "direction"), "down");
        QCOMPARE(SocketProtocol::v1Arg(args, "missing", "default"), "default");
    }

    void testV1HasFlag()
    {
        QStringList args = {"--verbose", "--cwd=/tmp"};
        QVERIFY(SocketProtocol::v1HasFlag(args, "verbose"));
        QVERIFY(!SocketProtocol::v1HasFlag(args, "quiet"));
    }
};

QTEST_GUILESS_MAIN(TestSocketProtocol)
#include "tst_SocketProtocol.moc"
