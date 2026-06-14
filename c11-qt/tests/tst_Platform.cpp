#include <QTest>
#include "platform/PlatformAbstraction.h"

class TestPlatform : public QObject {
    Q_OBJECT

private slots:
    void testSocketPathNotEmpty()
    {
        QString path = c11::platform::socketPath();
        QVERIFY(!path.isEmpty());
    }

    void testAppDataDirNotEmpty()
    {
        QString dir = c11::platform::appDataDir();
        QVERIFY(!dir.isEmpty());
    }

    void testConfigDirNotEmpty()
    {
        QString dir = c11::platform::configDir();
        QVERIFY(!dir.isEmpty());
    }

    void testSocketPathEnvOverride()
    {
        qputenv("C11_SOCKET", "/tmp/test-c11.sock");
        QCOMPARE(c11::platform::socketPath(), "/tmp/test-c11.sock");
        qunsetenv("C11_SOCKET");
    }
};

QTEST_GUILESS_MAIN(TestPlatform)
#include "tst_Platform.moc"
