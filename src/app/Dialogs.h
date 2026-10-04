#pragma once

#include "ui/KeymapPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <string>

namespace drumprog::app
{

/// Humble functions (E-03) opening the non-modal dialogs of the menus (Pflichtenheft 6.2).

/// Audio/MIDI settings: driver, device, sample rate, buffer size and MIDI inputs (F-AO-01, F-IN-03).
void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Component* centreAround);

/// Keyboard mapping with learning mode (F-IN-02).
void showKeymapDialog(ui::KeymapPresenter& presenter, juce::Component* centreAround);

/// Asks for one line of text, e.g. a pattern name; onOk gets the text unless the user cancels.
void askForText(const juce::String& title,
                const juce::String& label,
                const juce::String& initialText,
                std::function<void(const std::string&)> onOk);

} // namespace drumprog::app
