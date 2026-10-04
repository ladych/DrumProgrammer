#include "input/KeyboardInput.h"

namespace drumprog::input
{

KeyboardInput::KeyboardInput(const Keymap& keymap, INoteSink& sink, InputActivity& activity)
    : keymap_(keymap), sink_(sink), activity_(activity)
{
}

bool KeyboardInput::keyDown(int scancode, KeyModifiers modifiers, bool textFieldHasFocus)
{
    if (textFieldHasFocus || modifiers.commandOrAlt)
        return false;
    const auto notes = keymap_.notesFor(scancode);
    if (notes.empty())
        return false;
    if (!held_.insert(scancode).second)
        return true;

    const int velocity = modifiers.shift ? kAccentVelocity : kVelocity;
    for (const int note : notes)
        sink_.noteOn(note, velocity);
    activity_.signalKeyboard();
    return true;
}

void KeyboardInput::releaseKeysNotDown(const std::function<bool(int scancode)>& isDown)
{
    std::erase_if(held_, [&isDown](int scancode) { return !isDown(scancode); });
}

} // namespace drumprog::input
