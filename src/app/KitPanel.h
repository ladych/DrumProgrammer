#pragma once

#include "ui/KitPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace drumprog::app
{

/// Humble object (E-03): drum kit list with trigger LEDs (F-SE-04, F-SE-09) and the
/// sample part of the inspector (F-SE-05 to F-SE-07, F-SE-10). All logic is in ui::KitPresenter.
class KitPanel final : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    KitPanel(ui::KitPresenter& presenter, juce::String sampleWildcard);
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
    juce::String sampleWildcard_;
    std::vector<int> rows_;
    std::unique_ptr<juce::FileChooser> fileChooser_;

    juce::ListBox slotList_{"Drum-Kit", this};
    juce::ToggleButton showAllButton_{"Alle Slots anzeigen"};
    juce::Label slotName_;
    juce::Label sampleName_;
    juce::TextButton loadButton_{"Laden..."};
    juce::TextButton previewButton_{juce::String::fromUTF8("Vorh\xc3\xb6ren")};
    juce::Slider gainSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Slider pitchSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    juce::Slider noteSlider_{juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft};
    juce::Label gainLabel_{{}, juce::String::fromUTF8("Lautst\xc3\xa4rke (dB)")};
    juce::Label pitchLabel_{{}, juce::String::fromUTF8("Pitch (Halbt\xc3\xb6ne)")};
    juce::Label noteLabel_{{}, "MIDI-Note"};

    JUCE_LEAK_DETECTOR(KitPanel)
};

} // namespace drumprog::app
