#include <QTest>
#include <QStandardPaths>
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

    void testDefaultShellCommand()
    {
        const QString shell = c11::platform::defaultShellCommand();
#ifdef Q_OS_WIN
        // Windows must spawn PowerShell, never ghostty's bare cmd.exe default,
        // and the chosen binary must actually be resolvable on PATH.
        QVERIFY(shell == QStringLiteral("pwsh.exe")
                || shell == QStringLiteral("powershell.exe"));
        const QString base = shell.left(shell.lastIndexOf('.'));
        QVERIFY(!QStandardPaths::findExecutable(base).isEmpty());
#else
        // Elsewhere ghostty picks the login shell, so we deliberately return empty.
        QVERIFY(shell.isEmpty());
#endif
    }
};

QTEST_GUILESS_MAIN(TestPlatform)
#include "tst_Platform.moc"
