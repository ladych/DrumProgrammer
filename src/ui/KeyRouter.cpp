#include "ui/KeyRouter.h"

#include "input/Scancode.h"

namespace drumprog::ui
{

KeyRouter::KeyRouter(KeymapPresenter& keymap, input::KeyboardInput& keyboard, ITransportControl& transport)
    : keymap_(keymap), keyboard_(keyboard), transport_(transport)
{
}

bool KeyRouter::keyDown(int scancode, input::KeyModifiers modifiers, bool textFieldHasFocus)
{
    if (scancode == input::scancode::kSpace && !modifiers.commandOrAlt && !textFieldHasFocus)
    {
        transport_.togglePlay();
        return true;
    }
    if (keymap_.captureKey(scancode))
        return true;
    return keyboard_.keyDown(scancode, modifiers, textFieldHasFocus);
}

void KeyRouter::releaseKeysNotDown(const std::function<bool(int scancode)>& isDown)
{
    keyboard_.releaseKeysNotDown(isDown);
}

} // namespace drumprog::ui
