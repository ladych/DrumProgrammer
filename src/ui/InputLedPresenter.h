#pragma once

#include "input/InputActivity.h"
#include "ui/ActivityLed.h"

namespace drumprog::ui
{

/// Logic of the input LEDs "MIDI In" and "Tastatur" in the toolbar (F-IN-05). GUI thread only.
class InputLedPresenter
{
public:
    explicit InputLedPresenter(const input::InputActivity& activity);

    /// Call from the GUI timer.
    void tick();
    [[nodiscard]] bool isMidiLedOn() const noexcept { return midi_.isOn(); }
    [[nodiscard]] bool isKeyboardLedOn() const noexcept { return keyboard_.isOn(); }

private:
    const input::InputActivity& activity_;
    ActivityLed midi_;
    ActivityLed keyboard_;
};

} // namespace drumprog::ui
