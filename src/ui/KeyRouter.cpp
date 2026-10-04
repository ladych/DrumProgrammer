#include "ui/KeyRouter.h"

namespace drumprog::ui
{

KeyRouter::KeyRouter(KeymapPresenter& keymap, input::KeyboardInput& keyboard)
    : keymap_(keymap), keyboard_(keyboard)
{
}

bool KeyRouter::keyDown(int scancode, input::KeyModifiers modifiers, bool textFieldHasFocus)
{
    if (keymap_.captureKey(scancode))
        return true;
    return keyboard_.keyDown(scancode, modifiers, textFieldHasFocus);
}

void KeyRouter::releaseKeysNotDown(const std::function<bool(int scancode)>& isDown)
{
    keyboard_.releaseKeysNotDown(isDown);
}

} // namespace drumprog::ui
