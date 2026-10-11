#pragma once

#include "i18n/Translate.h"
#include "ui/KeymapPresenter.h"
#include "ui/KitPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace drumprog::app
{

/// Humble object (E-03), content of the kit dialog: drum kit list with trigger LEDs (F-SE-04, F-SE-09) and
/// the sample part of the inspector (F-SE-05 to F-SE-07, F-SE-10) with the slot's key (F-IN-02). All logic is
/// in ui::KitPresenter and ui::KeymapPresenter.
class KitPanel final : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    KitPanel(ui::KitPresenter& presenter, ui::KeymapPresenter& keymap, juce::String sampleWildcard);
    ~KitPanel() override;

    KitPanel(const KitPanel&) = delete;
    KitPanel& operator=(const KitPanel&) = delete;
    KitPanel(KitPanel&&) = delete;
    KitPanel& operator=(KitPanel&&) = delete;

    void resized() override;

private:
    int getNumRows() override;
    void
    paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void timerCallback() override;

    void setUpControls();
    void refreshRows();
    void refreshInspector();
    void chooseSample();

    ui::KitPresenter& presenter_;
    ui::KeymapPresenter& keymap_;
    juce::String sampleWildcard_;
    std::uint32_t seenChangeCount_ = 0;
    std::vector<int> rows_;
    std::unique_ptr<juce::FileChooser> fileChooser_;

    juce::Label kitSource_;
    juce::ListBox slotList_{i18n::trJuce("Drum kit"), this};
    juce::ToggleButton showAllButton_{i18n::trJuce("Show all slots")};
    juce::Label slotName_;
    juce::Label sampleName_;
    juce::TextButton loadButton_{i18n::trJuce("Load...")};
    juce::TextButton previewButton_{i18n::trJuce("Preview")};
    juce::Slider gainSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Slider pitchSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Slider noteSlider_{juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft};
    juce::TextButton keyButton_;
    juce::Label gainLabel_{{}, i18n::trJuce("Volume (dB)")};
    juce::Label pitchLabel_{{}, i18n::trJuce("Pitch (semitones)")};
    juce::Label noteLabel_{{}, i18n::trJuce("MIDI note")};
    juce::Label keyLabel_{{}, i18n::trJuce("Key (click, then press a key)")};

    JUCE_LEAK_DETECTOR(KitPanel)
};

} // namespace drumprog::app
