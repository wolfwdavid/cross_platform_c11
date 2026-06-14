#pragma once

#include "ghostty.h"
#include <QKeyEvent>
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

private:
    void buildKeyMap();

    std::unordered_map<int, ghostty_input_key_e> m_keyMap;
};

} // namespace c11
