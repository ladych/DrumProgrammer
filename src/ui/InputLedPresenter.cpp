#include "ui/InputLedPresenter.h"

namespace drumprog::ui
{

InputLedPresenter::InputLedPresenter(const input::InputActivity& activity) : activity_(activity) {}

void InputLedPresenter::tick()
{
    midi_.update(activity_.midiCount());
    keyboard_.update(activity_.keyboardCount());
}

} // namespace drumprog::ui
