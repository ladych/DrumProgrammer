#include "app/NoteInspector.h"

#include <array>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kLabelWidth = 80;

} // namespace

NoteInspector::NoteInspector(ui::PianoRollPresenter& presenter) : presenter_(presenter)
{
    title_.setFont(juce::FontOptions(15.0F, juce::Font::bold));
    velocity_.setRange(1.0, 127.0, 1.0);
    velocity_.onValueChange = [this]
    { presenter_.setSelectedVelocity(static_cast<int>(velocity_.getValue())); };
    velocity_.onDragEnd = [this] { presenter_.endVelocityEdit(); };
    for (auto* component : std::initializer_list<juce::Component*>{&title_,
                                                                   &instrumentLabel_,
                                                                   &positionLabel_,
                                                                   &lengthLabel_,
                                                                   &velocityLabel_,
                                                                   &instrument_,
                                                                   &position_,
                                                                   &length_,
                                                                   &velocity_})
    {
        component->setWantsKeyboardFocus(false);
        addAndMakeVisible(component);
    }
    refresh();
    startTimerHz(kTimerHz);
}

NoteInspector::~NoteInspector()
{
    stopTimer();
}

void NoteInspector::resized()
{
    auto area = getLocalBounds();
    title_.setBounds(area.removeFromTop(26));
    const std::array<std::pair<juce::Label*, juce::Component*>, 4> rows{{{&instrumentLabel_, &instrument_},
                                                                         {&positionLabel_, &position_},
                                                                         {&lengthLabel_, &length_},
                                                                         {&velocityLabel_, &velocity_}}};
    for (const auto& [label, value] : rows)
    {
        auto row = area.removeFromTop(28);
        label->setBounds(row.removeFromLeft(kLabelWidth));
        value->setBounds(row);
    }
}

void NoteInspector::timerCallback()
{
    if (presenter_.changeCount() != seenChangeCount_)
        refresh();
}

void NoteInspector::refresh()
{
    seenChangeCount_ = presenter_.changeCount();
    const auto details = presenter_.noteDetails();
    instrument_.setText(details ? juce::String::fromUTF8(details->instrument.c_str()) : "-",
                        juce::dontSendNotification);
    position_.setText(details ? juce::String::fromUTF8(details->position.c_str()) : "-",
                      juce::dontSendNotification);
    length_.setText(details ? juce::String::fromUTF8(details->length.c_str()) : "-",
                    juce::dontSendNotification);
    velocity_.setEnabled(details.has_value());
    if (details && !velocity_.isMouseButtonDown())
        velocity_.setValue(details->velocity, juce::dontSendNotification);
}

} // namespace drumprog::app
