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
};

QTEST_GUILESS_MAIN(TestGhosttyKeyMapper)
#include "tst_GhosttyKeyMapper.moc"
