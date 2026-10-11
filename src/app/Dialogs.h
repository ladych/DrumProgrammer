#pragma once

#include "ui/AboutInfo.h"
#include "ui/KeymapPresenter.h"
#include "ui/KitPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <string>
#include <vector>

namespace drumprog::app
{

/// Humble functions (E-03) opening the non-modal dialogs of the menus (Pflichtenheft 6.2).

/// Audio/MIDI settings: driver, device, sample rate, buffer size and MIDI inputs (F-AO-01, F-IN-03).
void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Component* centreAround);

/// Keyboard mapping with learning mode (F-IN-02).
void showKeymapDialog(ui::KeymapPresenter& presenter, juce::Component* centreAround);

/// Kit: slots with samples, gain, pitch, MIDI note and key (F-SE-04 to F-SE-10). The drum keys and the
/// shortcuts of the main window also work in it. Returns the window, which deletes itself when closed.
juce::DialogWindow* showKitDialog(ui::KitPresenter& kit,
                                  ui::KeymapPresenter& keymap,
                                  const juce::String& sampleWildcard,
                                  const std::vector<juce::KeyListener*>& keyListeners,
                                  juce::Component* centreAround);

/// Info: program, license, source code and third-party components with their licenses (Q-12).
void showAboutDialog(const ui::AboutInfo& info, juce::Component* centreAround);

/// Asks for one line of text, e.g. a pattern name; onOk gets the text unless the user cancels.
void askForText(const juce::String& title,
                const juce::String& label,
                const juce::String& initialText,
                std::function<void(const std::string&)> onOk);

/// Dialog "Neues Pattern" (AP14): asks for name and length in bars; onOk gets both unless the user cancels.
void askForNewPattern(const juce::String& name,
                      int bars,
                      int maxBars,
                      std::function<void(const std::string& name, int bars)> onOk);

/// Asks a yes/no question, e.g. before deleting; onYes runs only if the user confirms.
void askToConfirm(const juce::String& title,
                  const juce::String& question,
                  const juce::String& yesButton,
                  std::function<void()> onYes);

} // namespace drumprog::app
