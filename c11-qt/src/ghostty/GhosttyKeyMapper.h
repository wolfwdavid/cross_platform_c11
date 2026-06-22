#pragma once

#include "ghostty.h"
#include <QKeyEvent>
#include <QString>
#include <unordered_map>

namespace c11 {

// Translates Qt key events to Ghostty's input_key_s.
// Based on W3C UIEvents-code spec (same as Ghostty's key enum).
class GhosttyKeyMapper {
public:
    GhosttyKeyMapper();

    ghostty_input_key_s mapKeyEvent(const QKeyEvent *event,
                                     ghostty_input_action_e action) const;

    ghostty_input_key_e mapQtKey(int qtKey) const;
    ghostty_input_mods_e mapModifiers(Qt::KeyboardModifiers mods) const;

    // A parsed key chord ready to feed into ghostty_surface_key. `keycode` is the
    // platform-native scancode ghostty matches against its keycode table to
    // recover the logical key; `mods` is the modifier mask; `unshifted_codepoint`
    // is the base character for printable keys (0 for named keys).
    struct Chord {
        uint32_t keycode = 0;
        ghostty_input_mods_e mods = GHOSTTY_MODS_NONE;
        uint32_t unshifted_codepoint = 0;
    };

    // Parse a chord string like "ctrl+c", "enter", "shift+tab", "alt+f4" into a
    // Chord. Case-insensitive; '+'-separated; all but the last token are
    // modifiers (ctrl/control, alt/option, shift, super/cmd/win/meta). Returns
    // false if the key token is unrecognized. Pure/static — unit-testable.
    static bool parseChord(const QString &chord, Chord &out);

private:
    void buildKeyMap();

    std::unordered_map<int, ghostty_input_key_e> m_keyMap;
};

} // namespace c11
