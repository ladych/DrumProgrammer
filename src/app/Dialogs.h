#pragma once

#include "ui/KeymapPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace drumprog::app
{

/// Humble functions (E-03) opening the non-modal dialogs of the menu "Audio" (Pflichtenheft 6.2).

/// Audio/MIDI settings: driver, device, sample rate, buffer size and MIDI inputs (F-AO-01, F-IN-03).
void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Component* centreAround);

/// Keyboard mapping with learning mode (F-IN-02).
void showKeymapDialog(ui::KeymapPresenter& presenter, juce::Component* centreAround);

} // namespace drumprog::app
