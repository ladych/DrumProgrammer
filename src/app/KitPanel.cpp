#include "app/KitPanel.h"

#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kRowHeight = 24;
constexpr int kListWidth = 340;
constexpr int kLedSize = 10;

} // namespace

KitPanel::KitPanel(ui::KitPresenter& presenter, ui::KeymapPresenter& keymap, juce::String sampleWildcard)
    : presenter_(presenter), keymap_(keymap), sampleWildcard_(std::move(sampleWildcard)),
      seenChangeCount_(presenter.changeCount())
{
    setUpControls();
    for (auto* component : std::initializer_list<juce::Component*>{&slotList_,
                                                                   &showAllButton_,
                                                                   &slotName_,
                                                                   &sampleName_,
                                                                   &loadButton_,
                                                                   &previewButton_,
                                                                   &gainSlider_,
                                                                   &pitchSlider_,
                                                                   &noteSlider_,
                                                                   &keyButton_,
                                                                   &gainLabel_,
                                                                   &pitchLabel_,
                                                                   &noteLabel_,
                                                                   &keyLabel_})
        addAndMakeVisible(component);

    refreshRows();
    refreshInspector();
    startTimerHz(kTimerHz);
}

void KitPanel::setUpControls()
{
    slotList_.setRowHeight(kRowHeight);
    showAllButton_.onClick = [this] { refreshRows(); };
    loadButton_.onClick = [this] { chooseSample(); };
    previewButton_.onClick = [this] { presenter_.previewSelected(); };

    gainSlider_.setRange(ui::KitPresenter::kMinGainDb, ui::KitPresenter::kMaxGainDb, 0.1);
    gainSlider_.onValueChange = [this] { presenter_.setGainDb(static_cast<float>(gainSlider_.getValue())); };
    pitchSlider_.setRange(
        -engine::KitBuilder::kMaxPitchSemitones, engine::KitBuilder::kMaxPitchSemitones, 1.0);
    pitchSlider_.onValueChange = [this] { presenter_.setPitch(static_cast<int>(pitchSlider_.getValue())); };
    noteSlider_.setRange(0.0, 127.0, 1.0);
    noteSlider_.onValueChange = [this]
    {
        presenter_.setMidiNote(static_cast<int>(noteSlider_.getValue()));
        slotList_.repaint();
    };
    keyButton_.onClick = [this]
    {
        if (const auto selected = presenter_.selectedSlot())
            keymap_.startLearning(*selected);
    };
}

KitPanel::~KitPanel()
{
    stopTimer();
}

void KitPanel::resized()
{
    auto area = getLocalBounds();
    auto list = area.removeFromLeft(kListWidth);
    showAllButton_.setBounds(list.removeFromBottom(28));
    slotList_.setBounds(list);

    auto inspector = area.reduced(12, 0);
    slotName_.setBounds(inspector.removeFromTop(24));
    sampleName_.setBounds(inspector.removeFromTop(24));
    auto buttons = inspector.removeFromTop(28);
    loadButton_.setBounds(buttons.removeFromLeft(100));
    previewButton_.setBounds(buttons.removeFromLeft(100).withTrimmedLeft(8));
    for (auto [label, control] : {std::pair<juce::Label*, juce::Component*>{&gainLabel_, &gainSlider_},
                                  std::pair<juce::Label*, juce::Component*>{&pitchLabel_, &pitchSlider_},
                                  std::pair<juce::Label*, juce::Component*>{&noteLabel_, &noteSlider_},
                                  std::pair<juce::Label*, juce::Component*>{&keyLabel_, &keyButton_}})
    {
        inspector.removeFromTop(8);
        label->setBounds(inspector.removeFromTop(20));
        control->setBounds(inspector.removeFromTop(24).removeFromLeft(240));
    }
}

int KitPanel::getNumRows()
{
    return static_cast<int>(rows_.size());
}

void KitPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return;
    const int slot = rows_[static_cast<size_t>(rowNumber)];
    const auto& laf = getLookAndFeel();
    if (rowIsSelected)
        g.fillAll(laf.findColour(juce::TextEditor::highlightColourId));

    const auto led =
        juce::Rectangle<float>(6.0F, (static_cast<float>(height) - kLedSize) / 2.0F, kLedSize, kLedSize);
    g.setColour(presenter_.isLedOn(slot) ? juce::Colours::orange : juce::Colours::darkgrey);
    g.fillEllipse(led);

    g.setColour(laf.findColour(juce::Label::textColourId));
    g.drawText(presenter_.noteLabel(slot), 24, 0, 110, height, juce::Justification::centredLeft);
    g.drawText(juce::String::fromUTF8(presenter_.slotName(slot).c_str()),
               134,
               0,
               110,
               height,
               juce::Justification::centredLeft);
    g.drawText(juce::String::fromUTF8(presenter_.sampleLabel(slot).c_str()),
               244,
               0,
               width - 244,
               height,
               juce::Justification::centredLeft);
}

void KitPanel::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < getNumRows())
        presenter_.select(rows_[static_cast<size_t>(lastRowSelected)]);
    refreshInspector();
}

void KitPanel::timerCallback()
{
    presenter_.tick();
    // Undo, redo and loading a project change the kit behind the panel's back.
    if (presenter_.changeCount() != seenChangeCount_)
    {
        seenChangeCount_ = presenter_.changeCount();
        refreshRows();
        refreshInspector();
    }
    if (const auto selected = presenter_.selectedSlot())
        keyButton_.setButtonText(juce::String::fromUTF8(keymap_.keyLabel(*selected).c_str()));
    slotList_.repaint();
}

void KitPanel::refreshRows()
{
    rows_ = presenter_.visibleSlots(showAllButton_.getToggleState());
    slotList_.updateContent();
    slotList_.repaint();
}

void KitPanel::refreshInspector()
{
    const auto selected = presenter_.selectedSlot();
    for (auto* component : std::initializer_list<juce::Component*>{
             &loadButton_, &previewButton_, &gainSlider_, &pitchSlider_, &noteSlider_, &keyButton_})
        component->setEnabled(selected.has_value());
    if (!selected)
    {
        slotName_.setText(juce::String::fromUTF8("Kein Slot ausgew\xc3\xa4hlt"), juce::dontSendNotification);
        sampleName_.setText({}, juce::dontSendNotification);
        keyButton_.setButtonText({});
        return;
    }
    slotName_.setText(juce::String::fromUTF8(presenter_.slotName(*selected).c_str()),
                      juce::dontSendNotification);
    sampleName_.setText(juce::String::fromUTF8(presenter_.sampleLabel(*selected).c_str()),
                        juce::dontSendNotification);
    gainSlider_.setValue(presenter_.gainDb(), juce::dontSendNotification);
    pitchSlider_.setValue(presenter_.pitch(), juce::dontSendNotification);
    noteSlider_.setValue(presenter_.midiNote(*selected), juce::dontSendNotification);
    keyButton_.setButtonText(juce::String::fromUTF8(keymap_.keyLabel(*selected).c_str()));
}

void KitPanel::chooseSample()
{
    fileChooser_ = std::make_unique<juce::FileChooser>("Sample laden", juce::File{}, sampleWildcard_);
    fileChooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file == juce::File{})
                return;
            const auto utf8 = file.getFullPathName().toStdString();
            if (!presenter_.loadSample(std::u8string{utf8.begin(), utf8.end()}))
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                       "Sample laden",
                                                       "Die Datei konnte nicht geladen werden:\n" +
                                                           file.getFullPathName());
            refreshInspector();
            slotList_.repaint();
        });
}

} // namespace drumprog::app
