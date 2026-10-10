#pragma once

#include "app/KitPanel.h"
#include "app/MixPanel.h"
#include "app/NoteInspector.h"
#include "app/PatternListPanel.h"
#include "app/PianoRollToolbar.h"
#include "app/PianoRollView.h"
#include "app/SongTimelineView.h"
#include "app/TransportBar.h"
#include "engine/TestToneSource.h"
#include "ui/InputLedPresenter.h"
#include "ui/KeymapPresenter.h"
#include "ui/KitPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace drumprog::app
{

/// Humble object (E-03): main window content (Pflichtenheft 6.1) with the transport bar (F-TR-01 to 09),
/// the piano roll tools, the test tone switch, the input LEDs "MIDI In" and "Tastatur" (F-IN-05), the
/// pattern list (F-SO-01), the song timeline (F-SO-02 to 07), the piano roll with velocity lane (F-PR-01 to
/// 11), the note inspector with the mix faders (F-BT-06), the kit panel and a status line with device and
/// latency (Q-02) and, on the right, snap and selection. The song timeline includes the backing track lane
/// (F-BT-01 to 05).
class MainComponent final : public juce::Component,
                            public juce::DragAndDropContainer,
                            private juce::ChangeListener,
                            private juce::Timer
{
public:
    MainComponent(engine::TestToneSource& testTone,
                  juce::AudioDeviceManager& deviceManager,
                  ui::TransportPresenter& transport,
                  ui::TempoPresenter& tempo,
                  ui::KitPresenter& kitPresenter,
                  ui::PatternListPresenter& patterns,
                  PatternDialogs patternDialogs,
                  ui::SongTimelinePresenter& songTimeline,
                  ui::BackingTrackPresenter& backingTrack,
                  juce::AudioFormatManager& backingFormats,
                  std::function<void()> loadBackingTrack,
                  ui::MixPresenter& mix,
                  ui::PianoRollPresenter& pianoRoll,
                  ui::KeymapPresenter& keymapPresenter,
                  ui::InputLedPresenter& inputLeds,
                  const juce::String& sampleWildcard);
    ~MainComponent() override;

    MainComponent(const MainComponent&) = delete;
    MainComponent& operator=(const MainComponent&) = delete;
    MainComponent(MainComponent&&) = delete;
    MainComponent& operator=(MainComponent&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;
    void updateStatus();
    void paintLed(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& label, bool on) const;

    engine::TestToneSource& testTone_;
    juce::AudioDeviceManager& deviceManager_;
    ui::InputLedPresenter& inputLeds_;
    juce::ToggleButton testToneButton_{"Testton 440 Hz"};
    juce::TextButton settingsButton_{"Audio/MIDI-Einstellungen..."};
    ui::PianoRollPresenter& pianoRoll_;
    juce::Rectangle<int> ledArea_;
    TransportBar transportBar_;
    PianoRollToolbar pianoRollToolbar_;
    PatternListPanel patternList_;
    SongTimelineView songTimeline_;
    PianoRollView pianoRollView_;
    NoteInspector noteInspector_;
    MixPanel mixPanel_;
    KitPanel kitPanel_;
    juce::Label status_;
    juce::Label editStatus_;

    JUCE_LEAK_DETECTOR(MainComponent)
};

} // namespace drumprog::app
