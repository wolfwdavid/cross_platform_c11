#include "GhosttyKeyMapper.h"

#include <QStringList>

namespace c11 {

namespace {

// Native scancode for one key across the three platforms ghostty supports
// (matching the columns of ghostty's src/input/keycodes.zig: win, xkb, mac).
// ghostty recovers the logical key by matching the event keycode against these.
struct KeyScan {
    const char *name;
    uint32_t win;
    uint32_t xkb;
    uint32_t mac;
};

constexpr KeyScan kKeyScans[] = {
    // Letters
    {"a", 0x1e, 0x26, 0x00}, {"b", 0x30, 0x38, 0x0b}, {"c", 0x2e, 0x36, 0x08},
    {"d", 0x20, 0x28, 0x02}, {"e", 0x12, 0x1a, 0x0e}, {"f", 0x21, 0x29, 0x03},
    {"g", 0x22, 0x2a, 0x05}, {"h", 0x23, 0x2b, 0x04}, {"i", 0x17, 0x1f, 0x22},
    {"j", 0x24, 0x2c, 0x26}, {"k", 0x25, 0x2d, 0x28}, {"l", 0x26, 0x2e, 0x25},
    {"m", 0x32, 0x3a, 0x2e}, {"n", 0x31, 0x39, 0x2d}, {"o", 0x18, 0x20, 0x1f},
    {"p", 0x19, 0x21, 0x23}, {"q", 0x10, 0x18, 0x0c}, {"r", 0x13, 0x1b, 0x0f},
    {"s", 0x1f, 0x27, 0x01}, {"t", 0x14, 0x1c, 0x11}, {"u", 0x16, 0x1e, 0x20},
    {"v", 0x2f, 0x37, 0x09}, {"w", 0x11, 0x19, 0x0d}, {"x", 0x2d, 0x35, 0x07},
    {"y", 0x15, 0x1d, 0x10}, {"z", 0x2c, 0x34, 0x06},
    // Digits
    {"1", 0x02, 0x0a, 0x12}, {"2", 0x03, 0x0b, 0x13}, {"3", 0x04, 0x0c, 0x14},
    {"4", 0x05, 0x0d, 0x15}, {"5", 0x06, 0x0e, 0x17}, {"6", 0x07, 0x0f, 0x16},
    {"7", 0x08, 0x10, 0x1a}, {"8", 0x09, 0x11, 0x1c}, {"9", 0x0a, 0x12, 0x19},
    {"0", 0x0b, 0x13, 0x1d},
    // Named keys + common aliases (resolve to the same scancode)
    {"enter", 0x1c, 0x24, 0x24},      {"return", 0x1c, 0x24, 0x24},
    {"escape", 0x01, 0x09, 0x35},     {"esc", 0x01, 0x09, 0x35},
    {"backspace", 0x0e, 0x16, 0x33},  {"bs", 0x0e, 0x16, 0x33},
    {"tab", 0x0f, 0x17, 0x30},
    {"space", 0x39, 0x41, 0x31},
    {"delete", 0xe053, 0x77, 0x75},   {"del", 0xe053, 0x77, 0x75},
    {"insert", 0xe052, 0x76, 0x72},   {"ins", 0xe052, 0x76, 0x72},
    {"home", 0xe047, 0x6e, 0x73},     {"end", 0xe04f, 0x73, 0x77},
    {"pageup", 0xe049, 0x70, 0x74},   {"pgup", 0xe049, 0x70, 0x74},
    {"pagedown", 0xe051, 0x75, 0x79}, {"pgdn", 0xe051, 0x75, 0x79},
    {"up", 0xe048, 0x6f, 0x7e},       {"arrowup", 0xe048, 0x6f, 0x7e},
    {"down", 0xe050, 0x74, 0x7d},     {"arrowdown", 0xe050, 0x74, 0x7d},
    {"left", 0xe04b, 0x71, 0x7b},     {"arrowleft", 0xe04b, 0x71, 0x7b},
    {"right", 0xe04d, 0x72, 0x7c},    {"arrowright", 0xe04d, 0x72, 0x7c},
    {"f1", 0x3b, 0x43, 0x7a}, {"f2", 0x3c, 0x44, 0x78}, {"f3", 0x3d, 0x45, 0x63},
    {"f4", 0x3e, 0x46, 0x76}, {"f5", 0x3f, 0x47, 0x60}, {"f6", 0x40, 0x48, 0x61},
    {"f7", 0x41, 0x49, 0x62}, {"f8", 0x42, 0x4a, 0x64}, {"f9", 0x43, 0x4b, 0x65},
    {"f10", 0x44, 0x4c, 0x6d}, {"f11", 0x57, 0x5f, 0x67}, {"f12", 0x58, 0x60, 0x6f},
};

uint32_t pickNative(const KeyScan &k)
{
#if defined(Q_OS_WIN)
    return k.win;
#elif defined(Q_OS_MACOS)
    return k.mac;
#else
    return k.xkb;
#endif
}

} // namespace

GhosttyKeyMapper::GhosttyKeyMapper()
{
    buildKeyMap();
}

ghostty_input_key_s GhosttyKeyMapper::mapKeyEvent(const QKeyEvent *event,
                                                    ghostty_input_action_e action) const
{
    ghostty_input_key_s key{};
    key.action = action;
    key.mods = mapModifiers(event->modifiers());
    key.consumed_mods = GHOSTTY_MODS_NONE;
    key.keycode = static_cast<uint32_t>(event->nativeScanCode());
    key.composing = false;

    // Map the logical key
    auto mapped = mapQtKey(event->key());
    // The keycode field in ghostty_input_key_s is the physical scancode.
    // The translated key is communicated via the text field.
    Q_UNUSED(mapped);

    // Text for printable characters
    QString text = event->text();
    if (!text.isEmpty()) {
        // Store the unshifted codepoint
        key.unshifted_codepoint = text.at(0).unicode();
    }

    // text pointer - Ghostty copies this, so stack-local is fine within
    // the scope of the caller. We pass nullptr; the caller sends text
    // separately via ghostty_surface_text.
    key.text = nullptr;

    return key;
}

ghostty_input_key_e GhosttyKeyMapper::mapQtKey(int qtKey) const
{
    auto it = m_keyMap.find(qtKey);
    if (it != m_keyMap.end()) {
        return it->second;
    }
    return GHOSTTY_KEY_UNIDENTIFIED;
}

ghostty_input_mods_e GhosttyKeyMapper::mapModifiers(Qt::KeyboardModifiers mods) const
{
    int result = GHOSTTY_MODS_NONE;
    if (mods & Qt::ShiftModifier) result |= GHOSTTY_MODS_SHIFT;
    if (mods & Qt::ControlModifier) {
#ifdef Q_OS_MACOS
        result |= GHOSTTY_MODS_SUPER;
#else
        result |= GHOSTTY_MODS_CTRL;
#endif
    }
    if (mods & Qt::AltModifier) result |= GHOSTTY_MODS_ALT;
    if (mods & Qt::MetaModifier) {
#ifdef Q_OS_MACOS
        result |= GHOSTTY_MODS_CTRL;
#else
        result |= GHOSTTY_MODS_SUPER;
#endif
    }
    return static_cast<ghostty_input_mods_e>(result);
}

void GhosttyKeyMapper::buildKeyMap()
{
    // Letters
    m_keyMap[Qt::Key_A] = GHOSTTY_KEY_A;
    m_keyMap[Qt::Key_B] = GHOSTTY_KEY_B;
    m_keyMap[Qt::Key_C] = GHOSTTY_KEY_C;
    m_keyMap[Qt::Key_D] = GHOSTTY_KEY_D;
    m_keyMap[Qt::Key_E] = GHOSTTY_KEY_E;
    m_keyMap[Qt::Key_F] = GHOSTTY_KEY_F;
    m_keyMap[Qt::Key_G] = GHOSTTY_KEY_G;
    m_keyMap[Qt::Key_H] = GHOSTTY_KEY_H;
    m_keyMap[Qt::Key_I] = GHOSTTY_KEY_I;
    m_keyMap[Qt::Key_J] = GHOSTTY_KEY_J;
    m_keyMap[Qt::Key_K] = GHOSTTY_KEY_K;
    m_keyMap[Qt::Key_L] = GHOSTTY_KEY_L;
    m_keyMap[Qt::Key_M] = GHOSTTY_KEY_M;
    m_keyMap[Qt::Key_N] = GHOSTTY_KEY_N;
    m_keyMap[Qt::Key_O] = GHOSTTY_KEY_O;
    m_keyMap[Qt::Key_P] = GHOSTTY_KEY_P;
    m_keyMap[Qt::Key_Q] = GHOSTTY_KEY_Q;
    m_keyMap[Qt::Key_R] = GHOSTTY_KEY_R;
    m_keyMap[Qt::Key_S] = GHOSTTY_KEY_S;
    m_keyMap[Qt::Key_T] = GHOSTTY_KEY_T;
    m_keyMap[Qt::Key_U] = GHOSTTY_KEY_U;
    m_keyMap[Qt::Key_V] = GHOSTTY_KEY_V;
    m_keyMap[Qt::Key_W] = GHOSTTY_KEY_W;
    m_keyMap[Qt::Key_X] = GHOSTTY_KEY_X;
    m_keyMap[Qt::Key_Y] = GHOSTTY_KEY_Y;
    m_keyMap[Qt::Key_Z] = GHOSTTY_KEY_Z;

    // Digits
    m_keyMap[Qt::Key_0] = GHOSTTY_KEY_DIGIT_0;
    m_keyMap[Qt::Key_1] = GHOSTTY_KEY_DIGIT_1;
    m_keyMap[Qt::Key_2] = GHOSTTY_KEY_DIGIT_2;
    m_keyMap[Qt::Key_3] = GHOSTTY_KEY_DIGIT_3;
    m_keyMap[Qt::Key_4] = GHOSTTY_KEY_DIGIT_4;
    m_keyMap[Qt::Key_5] = GHOSTTY_KEY_DIGIT_5;
    m_keyMap[Qt::Key_6] = GHOSTTY_KEY_DIGIT_6;
    m_keyMap[Qt::Key_7] = GHOSTTY_KEY_DIGIT_7;
    m_keyMap[Qt::Key_8] = GHOSTTY_KEY_DIGIT_8;
    m_keyMap[Qt::Key_9] = GHOSTTY_KEY_DIGIT_9;

    // Punctuation / writing system keys
    m_keyMap[Qt::Key_QuoteLeft]    = GHOSTTY_KEY_BACKQUOTE;
    m_keyMap[Qt::Key_Backslash]    = GHOSTTY_KEY_BACKSLASH;
    m_keyMap[Qt::Key_BracketLeft]  = GHOSTTY_KEY_BRACKET_LEFT;
    m_keyMap[Qt::Key_BracketRight] = GHOSTTY_KEY_BRACKET_RIGHT;
    m_keyMap[Qt::Key_Comma]        = GHOSTTY_KEY_COMMA;
    m_keyMap[Qt::Key_Equal]        = GHOSTTY_KEY_EQUAL;
    m_keyMap[Qt::Key_Minus]        = GHOSTTY_KEY_MINUS;
    m_keyMap[Qt::Key_Period]       = GHOSTTY_KEY_PERIOD;
    m_keyMap[Qt::Key_Apostrophe]   = GHOSTTY_KEY_QUOTE;
    m_keyMap[Qt::Key_Semicolon]    = GHOSTTY_KEY_SEMICOLON;
    m_keyMap[Qt::Key_Slash]        = GHOSTTY_KEY_SLASH;

    // Functional keys
    m_keyMap[Qt::Key_Alt]       = GHOSTTY_KEY_ALT_LEFT;
    m_keyMap[Qt::Key_Backspace] = GHOSTTY_KEY_BACKSPACE;
    m_keyMap[Qt::Key_CapsLock]  = GHOSTTY_KEY_CAPS_LOCK;
    m_keyMap[Qt::Key_Menu]      = GHOSTTY_KEY_CONTEXT_MENU;
    m_keyMap[Qt::Key_Control]   = GHOSTTY_KEY_CONTROL_LEFT;
    m_keyMap[Qt::Key_Return]    = GHOSTTY_KEY_ENTER;
    m_keyMap[Qt::Key_Enter]     = GHOSTTY_KEY_ENTER;
    m_keyMap[Qt::Key_Meta]      = GHOSTTY_KEY_META_LEFT;
    m_keyMap[Qt::Key_Shift]     = GHOSTTY_KEY_SHIFT_LEFT;
    m_keyMap[Qt::Key_Space]     = GHOSTTY_KEY_SPACE;
    m_keyMap[Qt::Key_Tab]       = GHOSTTY_KEY_TAB;

    // Control pad
    m_keyMap[Qt::Key_Delete]   = GHOSTTY_KEY_DELETE;
    m_keyMap[Qt::Key_End]      = GHOSTTY_KEY_END;
    m_keyMap[Qt::Key_Home]     = GHOSTTY_KEY_HOME;
    m_keyMap[Qt::Key_Insert]   = GHOSTTY_KEY_INSERT;
    m_keyMap[Qt::Key_PageDown] = GHOSTTY_KEY_PAGE_DOWN;
    m_keyMap[Qt::Key_PageUp]   = GHOSTTY_KEY_PAGE_UP;

    // Arrow keys
    m_keyMap[Qt::Key_Down]  = GHOSTTY_KEY_ARROW_DOWN;
    m_keyMap[Qt::Key_Left]  = GHOSTTY_KEY_ARROW_LEFT;
    m_keyMap[Qt::Key_Right] = GHOSTTY_KEY_ARROW_RIGHT;
    m_keyMap[Qt::Key_Up]    = GHOSTTY_KEY_ARROW_UP;

    // Function keys
    m_keyMap[Qt::Key_Escape] = GHOSTTY_KEY_ESCAPE;
    m_keyMap[Qt::Key_F1]     = GHOSTTY_KEY_F1;
    m_keyMap[Qt::Key_F2]     = GHOSTTY_KEY_F2;
    m_keyMap[Qt::Key_F3]     = GHOSTTY_KEY_F3;
    m_keyMap[Qt::Key_F4]     = GHOSTTY_KEY_F4;
    m_keyMap[Qt::Key_F5]     = GHOSTTY_KEY_F5;
    m_keyMap[Qt::Key_F6]     = GHOSTTY_KEY_F6;
    m_keyMap[Qt::Key_F7]     = GHOSTTY_KEY_F7;
    m_keyMap[Qt::Key_F8]     = GHOSTTY_KEY_F8;
    m_keyMap[Qt::Key_F9]     = GHOSTTY_KEY_F9;
    m_keyMap[Qt::Key_F10]    = GHOSTTY_KEY_F10;
    m_keyMap[Qt::Key_F11]    = GHOSTTY_KEY_F11;
    m_keyMap[Qt::Key_F12]    = GHOSTTY_KEY_F12;
    m_keyMap[Qt::Key_F13]    = GHOSTTY_KEY_F13;
    m_keyMap[Qt::Key_F14]    = GHOSTTY_KEY_F14;
    m_keyMap[Qt::Key_F15]    = GHOSTTY_KEY_F15;
    m_keyMap[Qt::Key_F16]    = GHOSTTY_KEY_F16;
    m_keyMap[Qt::Key_F17]    = GHOSTTY_KEY_F17;
    m_keyMap[Qt::Key_F18]    = GHOSTTY_KEY_F18;
    m_keyMap[Qt::Key_F19]    = GHOSTTY_KEY_F19;
    m_keyMap[Qt::Key_F20]    = GHOSTTY_KEY_F20;
    m_keyMap[Qt::Key_Print]       = GHOSTTY_KEY_PRINT_SCREEN;
    m_keyMap[Qt::Key_ScrollLock]  = GHOSTTY_KEY_SCROLL_LOCK;
    m_keyMap[Qt::Key_Pause]       = GHOSTTY_KEY_PAUSE;
    m_keyMap[Qt::Key_NumLock]     = GHOSTTY_KEY_NUM_LOCK;

    // Media keys
    m_keyMap[Qt::Key_MediaPlay]         = GHOSTTY_KEY_MEDIA_PLAY_PAUSE;
    m_keyMap[Qt::Key_MediaStop]         = GHOSTTY_KEY_MEDIA_STOP;
    m_keyMap[Qt::Key_MediaNext]         = GHOSTTY_KEY_MEDIA_TRACK_NEXT;
    m_keyMap[Qt::Key_MediaPrevious]     = GHOSTTY_KEY_MEDIA_TRACK_PREVIOUS;
    m_keyMap[Qt::Key_VolumeDown]        = GHOSTTY_KEY_AUDIO_VOLUME_DOWN;
    m_keyMap[Qt::Key_VolumeMute]        = GHOSTTY_KEY_AUDIO_VOLUME_MUTE;
    m_keyMap[Qt::Key_VolumeUp]          = GHOSTTY_KEY_AUDIO_VOLUME_UP;
}

bool GhosttyKeyMapper::parseChord(const QString &chord, Chord &out)
{
    const QString norm = chord.trimmed().toLower();
    if (norm.isEmpty()) return false;

    const QStringList tokens = norm.split('+', Qt::KeepEmptyParts);
    // The last token is the key; everything before it is a modifier. An empty
    // key token (e.g. a trailing '+') is unsupported.
    const QString keyToken = tokens.last();
    if (keyToken.isEmpty()) return false;

    int mods = GHOSTTY_MODS_NONE;
    for (int i = 0; i < tokens.size() - 1; ++i) {
        const QString m = tokens[i].trimmed();
        if (m == "ctrl" || m == "control") mods |= GHOSTTY_MODS_CTRL;
        else if (m == "shift") mods |= GHOSTTY_MODS_SHIFT;
        else if (m == "alt" || m == "option" || m == "opt") mods |= GHOSTTY_MODS_ALT;
        else if (m == "super" || m == "cmd" || m == "command" || m == "win"
                 || m == "windows" || m == "meta") mods |= GHOSTTY_MODS_SUPER;
        else return false; // unknown modifier
    }

    for (const auto &k : kKeyScans) {
        if (keyToken == QLatin1String(k.name)) {
            out.keycode = pickNative(k);
            out.mods = static_cast<ghostty_input_mods_e>(mods);
            // Printable single-char keys carry their base codepoint, mirroring a
            // real keypress; named keys (enter, tab, …) carry none.
            out.unshifted_codepoint =
                (keyToken.size() == 1) ? keyToken.at(0).unicode() : 0;
            return true;
        }
    }
    return false; // unrecognized key
}

} // namespace c11
