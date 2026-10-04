#pragma once

#include "input/INoteSink.h"
#include "input/InputActivity.h"
#include "input/Keymap.h"

#include <functional>
#include <set>

namespace drumprog::input
{

struct KeyModifiers
{
    bool shift = false;
    bool commandOrAlt = false; ///< Ctrl, Cmd or Alt: a menu shortcut, never a drum hit (6.4)
};

/// Computer keyboard as drum trigger (F-IN-01, F-IN-06). Runs on the GUI thread and hands hits to
/// the sink, which queues them lock-free for the audio thread.
class KeyboardInput
{
public:
    static constexpr int kVelocity = 100;
    static constexpr int kAccentVelocity = 127; ///< with Shift

    KeyboardInput(const Keymap& keymap, INoteSink& sink, InputActivity& activity);

    /// A key was pressed. Triggers every note mapped to it unless a text field has the focus or a
    /// menu modifier is held. A key that is still held (auto-repeat) does not trigger again.
    /// Returns true if the key is a drum key and was used.
    bool keyDown(int scancode, KeyModifiers modifiers, bool textFieldHasFocus);
    /// Forgets held keys that are no longer down, so they trigger again on the next press.
    void releaseKeysNotDown(const std::function<bool(int scancode)>& isDown);
    [[nodiscard]] const std::set<int>& heldKeys() const noexcept { return held_; }

private:
    const Keymap& keymap_;
    INoteSink& sink_;
    InputActivity& activity_;
    std::set<int> held_;
};

} // namespace drumprog::input
