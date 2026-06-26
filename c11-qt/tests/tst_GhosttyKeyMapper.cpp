#include <QTest>
#include "ghostty/GhosttyKeyMapper.h"

using namespace c11;

class TestGhosttyKeyMapper : public QObject {
    Q_OBJECT

private slots:
    void testLetterKeys()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapQtKey(Qt::Key_A), GHOSTTY_KEY_A);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Z), GHOSTTY_KEY_Z);
    }

    void testDigitKeys()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapQtKey(Qt::Key_0), GHOSTTY_KEY_DIGIT_0);
        QCOMPARE(mapper.mapQtKey(Qt::Key_9), GHOSTTY_KEY_DIGIT_9);
    }

    void testFunctionKeys()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapQtKey(Qt::Key_F1), GHOSTTY_KEY_F1);
        QCOMPARE(mapper.mapQtKey(Qt::Key_F12), GHOSTTY_KEY_F12);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Escape), GHOSTTY_KEY_ESCAPE);
    }

    void testArrowKeys()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapQtKey(Qt::Key_Up), GHOSTTY_KEY_ARROW_UP);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Down), GHOSTTY_KEY_ARROW_DOWN);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Left), GHOSTTY_KEY_ARROW_LEFT);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Right), GHOSTTY_KEY_ARROW_RIGHT);
    }

    void testSpecialKeys()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapQtKey(Qt::Key_Return), GHOSTTY_KEY_ENTER);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Backspace), GHOSTTY_KEY_BACKSPACE);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Tab), GHOSTTY_KEY_TAB);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Space), GHOSTTY_KEY_SPACE);
        QCOMPARE(mapper.mapQtKey(Qt::Key_Delete), GHOSTTY_KEY_DELETE);
    }

    void testUnknownKey()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapQtKey(Qt::Key_unknown), GHOSTTY_KEY_UNIDENTIFIED);
    }

    void testModifiers()
    {
        GhosttyKeyMapper mapper;
        QCOMPARE(mapper.mapModifiers(Qt::NoModifier),
                 static_cast<ghostty_input_mods_e>(GHOSTTY_MODS_NONE));
        QVERIFY(mapper.mapModifiers(Qt::ShiftModifier) & GHOSTTY_MODS_SHIFT);
        QVERIFY(mapper.mapModifiers(Qt::AltModifier) & GHOSTTY_MODS_ALT);
    }

    // --- parseChord (powers `c11 send-key`) ---

    void testParseNamedKey()
    {
        GhosttyKeyMapper::Chord c;
        QVERIFY(GhosttyKeyMapper::parseChord("enter", c));
        QCOMPARE(c.mods, static_cast<ghostty_input_mods_e>(GHOSTTY_MODS_NONE));
        QCOMPARE(c.unshifted_codepoint, 0u);   // named key carries no codepoint
#if defined(Q_OS_WIN)
        QCOMPARE(c.keycode, 0x1Cu);            // Enter's Windows native scancode
#else
        QVERIFY(c.keycode != 0u);
#endif
    }

    void testParseCtrlLetter()
    {
        GhosttyKeyMapper::Chord c;
        QVERIFY(GhosttyKeyMapper::parseChord("ctrl+c", c));
        QVERIFY(c.mods & GHOSTTY_MODS_CTRL);
        QVERIFY(!(c.mods & GHOSTTY_MODS_SHIFT));
        QCOMPARE(c.unshifted_codepoint, static_cast<uint32_t>('c'));
#if defined(Q_OS_WIN)
        QCOMPARE(c.keycode, 0x2Eu);            // 'c' Windows scancode
#endif
    }

    void testParseCaseInsensitiveAndMultiMod()
    {
        GhosttyKeyMapper::Chord c;
        QVERIFY(GhosttyKeyMapper::parseChord("Ctrl+Shift+K", c));
        QVERIFY(c.mods & GHOSTTY_MODS_CTRL);
        QVERIFY(c.mods & GHOSTTY_MODS_SHIFT);
        QCOMPARE(c.unshifted_codepoint, static_cast<uint32_t>('k'));
    }

    void testParseShiftTab()
    {
        GhosttyKeyMapper::Chord c;
        QVERIFY(GhosttyKeyMapper::parseChord("shift+tab", c));
        QVERIFY(c.mods & GHOSTTY_MODS_SHIFT);
        QCOMPARE(c.unshifted_codepoint, 0u);
    }

    void testParseRejectsGarbage()
    {
        GhosttyKeyMapper::Chord c;
        QVERIFY(!GhosttyKeyMapper::parseChord("", c));
        QVERIFY(!GhosttyKeyMapper::parseChord("notakey", c));
        QVERIFY(!GhosttyKeyMapper::parseChord("ctrl+", c));      // empty key token
        QVERIFY(!GhosttyKeyMapper::parseChord("hyper+c", c));    // unknown modifier
    }
};

QTEST_GUILESS_MAIN(TestGhosttyKeyMapper)
#include "tst_GhosttyKeyMapper.moc"
