#include "GhosttyKeyMapper.h"

namespace c11 {

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

} // namespace c11
