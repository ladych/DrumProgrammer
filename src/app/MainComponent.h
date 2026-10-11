#pragma once

#include "app/AudioDeviceSupervisor.h"
#include "app/MetronomePanel.h"
#include "app/MixPanel.h"
#include "app/NoteInspector.h"
#include "app/PatternListPanel.h"
#include "app/PianoRollView.h"
#include "app/SongTimelineView.h"
#include "app/Splitter.h"
#include "app/TransportBar.h"
#include "ui/InputLedPresenter.h"
#include "ui/PaneSize.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <vector>

namespace drumprog::app
{

/// Humble object (E-03): main window content (Pflichtenheft 6.1, layout after AP14). On top one row with the
/// transport bar (F-TR-01 to 09) and, in the right corner, the input LEDs "MIDI In" and "Tastatur"
/// (F-IN-05). Below three columns with splitters between them: the pattern list (F-SO-01); the song
/// timeline with the backing track lane (F-SO-02 to 07, F-BT-01 to 05) above the piano roll with velocity
/// lane (F-PR-01 to 11), also behind a splitter; on the right metronome and recording settings next to the
/// song timeline and, level with the piano roll, the note inspector with the mix faders (F-BT-06). At the
/// bottom a status line with device and latency (Q-02) and, on the right, snap and selection. MIDI import
/// and export and the audio settings are in the menu.
class MainComponent final : public juce::Component,
                            public juce::DragAndDropContainer,
                            private juce::ChangeListener,
                            private juce::Timer
{
public:
    MainComponent(juce::AudioDeviceManager& deviceManager,
                  const AudioDeviceSupervisor& deviceSupervisor,
                  ui::TransportPresenter& transport,
                  ui::TempoPresenter& tempo,
                  ui::PatternListPresenter& patterns,
                  PatternDialogs patternDialogs,
                  ui::SongTimelinePresenter& songTimeline,
                  ui::BackingTrackPresenter& backingTrack,
                  juce::AudioFormatManager& backingFormats,
                  std::function<void()> loadBackingTrack,
                  ui::MixPresenter& mix,
                  ui::PianoRollPresenter& pianoRoll,
                  ui::InputLedPresenter& inputLeds);
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
    void setUpSplitters();
    void layOutColumns(juce::Rectangle<int> area);
    void updateStatus();
    void paintLed(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& label, bool on) const;
    [[nodiscard]] juce::Rectangle<int> bodyArea() const;

    juce::AudioDeviceManager& deviceManager_;
    const AudioDeviceSupervisor& deviceSupervisor_;
    int statusTicks_ = 0;
    ui::InputLedPresenter& inputLeds_;
    ui::PianoRollPresenter& pianoRoll_;
    juce::Rectangle<int> topBar_;
    juce::Rectangle<int> ledArea_;
    std::vector<juce::Rectangle<int>> panels_;
    ui::PaneSize patternColumn_;
    ui::PaneSize rightColumn_;
    ui::PaneSize songHeight_;
    TransportBar transportBar_;
    PatternListPanel patternList_;
    SongTimelineView songTimeline_;
    PianoRollView pianoRollView_;
    MetronomePanel metronomePanel_;
    NoteInspector noteInspector_;
    MixPanel mixPanel_;
    Splitter leftSplitter_{Splitter::Direction::leftRight};
    Splitter rightSplitter_{Splitter::Direction::leftRight};
    Splitter songSplitter_{Splitter::Direction::upDown};
    juce::Label status_;
    juce::Label editStatus_;
    juce::TooltipWindow tooltipWindow_{this};

    JUCE_LEAK_DETECTOR(MainComponent)
};

} // namespace drumprog::app
