#include <QTest>
#include "ghostty/GhosttyConfig.h"

using namespace c11;

class TestGhosttyConfig : public QObject {
    Q_OBJECT

private slots:
    void testDefaults()
    {
        GhosttyConfig config;
#if defined(Q_OS_WIN)
        QCOMPARE(config.fontFamily, "Cascadia Mono");
#elif defined(Q_OS_MACOS)
        QCOMPARE(config.fontFamily, "Menlo");
#else
        QCOMPARE(config.fontFamily, "monospace");
#endif
        QCOMPARE(config.fontSize, 12.0);
        QCOMPARE(config.scrollbackLimit, 10000);
        QCOMPARE(config.backgroundOpacity, 1.0);
        QVERIFY(config.backgroundColor.isValid());
        QVERIFY(config.foregroundColor.isValid());
    }

    void testParseBasic()
    {
        GhosttyConfig config;
        config.parse("font-family = JetBrains Mono\nfont-size = 14\nscrollback-limit = 5000\n");
        QCOMPARE(config.fontFamily, "JetBrains Mono");
        QCOMPARE(config.fontSize, 14.0);
        QCOMPARE(config.scrollbackLimit, 5000);
    }

    void testParseColors()
    {
        GhosttyConfig config;
        config.parse("background = #1a1b26\nforeground = #c0caf5\n");
        QCOMPARE(config.backgroundColor, QColor(0x1a, 0x1b, 0x26));
        QCOMPARE(config.foregroundColor, QColor(0xc0, 0xca, 0xf5));
    }

    void testSkipsComments()
    {
        GhosttyConfig config;
        config.parse("# comment\nfont-size = 16\n# another comment\n");
        QCOMPARE(config.fontSize, 16.0);
    }

    void testSkipsEmptyLines()
    {
        GhosttyConfig config;
        config.parse("\n\nfont-size = 18\n\n");
        QCOMPARE(config.fontSize, 18.0);
    }

    void testContrastFallback()
    {
        GhosttyConfig config;
        config.parse("background = #ffffff\nforeground = #eeeeee\n");
        // Both are light, so the fallback should kick in after load()
        // The parse itself doesn't apply it, but we can test manually
        config.applyContrastFallback();
        QCOMPARE(config.foregroundColor, QColor(0x1A, 0x1A, 0x1A));
    }

    void testContrastNoFallbackNeeded()
    {
        GhosttyConfig config;
        config.parse("background = #1a1a1a\nforeground = #ffffff\n");
        QColor originalFg = config.foregroundColor;
        config.applyContrastFallback();
        QCOMPARE(config.foregroundColor, originalFg);
    }

    void testBackgroundOpacity()
    {
        GhosttyConfig config;
        config.parse("background-opacity = 0.85\n");
        QCOMPARE(config.backgroundOpacity, 0.85);
    }

    void testUnfocusedSplitOverlay()
    {
        GhosttyConfig config;
        config.unfocusedSplitOpacity = 0.7;
        QVERIFY(qFuzzyCompare(config.unfocusedSplitOverlayOpacity(), 0.3));
    }

    void testQuotedValues()
    {
        GhosttyConfig config;
        config.parse("font-family = \"Fira Code\"\n");
        QCOMPARE(config.fontFamily, "Fira Code");
    }
};

QTEST_GUILESS_MAIN(TestGhosttyConfig)
#include "tst_GhosttyConfig.moc"
