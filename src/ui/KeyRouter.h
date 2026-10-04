#pragma once

#include "input/KeyboardInput.h"
#include "ui/ITransportControl.h"
#include "ui/KeymapPresenter.h"

#include <functional>

namespace drumprog::ui
{

/// Entry point for key presses of the main window: the space bar starts and stops the transport
/// (F-TR-01), a key the mapping is learning goes to the KeymapPresenter, every other key to the drum
/// keyboard. GUI thread only.
class KeyRouter
{
public:
    KeyRouter(KeymapPresenter& keymap, input::KeyboardInput& keyboard, ITransportControl& transport);

    /// Returns true if the key was used and must not be handled further.
    bool keyDown(int scancode, input::KeyModifiers modifiers, bool textFieldHasFocus);
    void releaseKeysNotDown(const std::function<bool(int scancode)>& isDown);

private:
    KeymapPresenter& keymap_;
    input::KeyboardInput& keyboard_;
    ITransportControl& transport_;
};

} // namespace drumprog::ui
