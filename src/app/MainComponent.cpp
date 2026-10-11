#include "app/MainComponent.h"

#include "app/Theme.h"
#include "ui/AudioStatus.h"

#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kLedSize = 10;
constexpr int kLedWidth = 90;
constexpr int kGap = Splitter::kThickness;
constexpr int kPadding = 10;
constexpr int kTopBarHeight = 46;
constexpr int kStatusHeight = 24;
constexpr int kPatternColumnWidth = 240;
constexpr int kPatternColumnMinimum = 150;
constexpr int kRightColumnWidth = 290;
constexpr int kRightColumnMinimum = 240;
constexpr int kCenterMinimum = 520;
constexpr int kSongMinimum = MetronomePanel::kMinimumHeight + kPadding;
constexpr int kPianoRollMinimum = 260;
constexpr int kNoteInspectorHeight = 26 + 4 * 28;
/// What the pattern column leaves free: the right column at its minimum, the centre and two splitters.
constexpr int kBesidePatternColumn = kRightColumnMinimum + kCenterMinimum + 2 * Splitter::kThickness;
constexpr int kBesideRightColumn = kCenterMinimum + Splitter::kThickness;
constexpr int kBelowSong = kPianoRollMinimum + Splitter::kThickness;

ui::AudioDeviceInfo deviceInfo(juce::AudioDeviceManager& deviceManager,
                               const AudioDeviceSupervisor& supervisor)
{
    ui::AudioDeviceInfo info;
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        info.open = true;
        info.driver = device->getTypeName().toStdString();
        info.device = device->getName().toStdString();
        info.sampleRate = device->getCurrentSampleRate();
        info.bufferSize = device->getCurrentBufferSizeSamples();
        info.outputLatencySamples = device->getOutputLatencyInSamples();
    }
    for (const auto& input : juce::MidiInput::getAvailableDevices())
        if (deviceManager.isMidiInputDeviceEnabled(input.identifier))
            info.midiInputs.push_back(input.name.toStdString());
    supervisor.describe(info);
    return info;
}

} // namespace

MainComponent::MainComponent(juce::AudioDeviceManager& deviceManager,
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
                             ui::InputLedPresenter& inputLeds)
    : deviceManager_(deviceManager), deviceSupervisor_(deviceSupervisor), inputLeds_(inputLeds),
      pianoRoll_(pianoRoll), patternColumn_(kPatternColumnWidth, kPatternColumnMinimum),
      rightColumn_(kRightColumnWidth, kRightColumnMinimum),
      songHeight_(SongTimelineView::kPreferredHeight, kSongMinimum), transportBar_(transport, tempo),
      patternList_(patterns, std::move(patternDialogs)),
      songTimeline_(songTimeline, transport, backingTrack, backingFormats, std::move(loadBackingTrack)),
      pianoRollView_(pianoRoll, transport, patterns), metronomePanel_(transport), noteInspector_(pianoRoll),
      mixPanel_(mix)
{
    for (auto* label : {&status_, &editStatus_})
    {
        label->setFont(juce::FontOptions{juce::Font::getDefaultMonospacedFontName(), 13.0F, 0});
        label->setColour(juce::Label::textColourId, theme::kTextDim);
    }
    editStatus_.setJustificationType(juce::Justification::centredRight);
    setUpSplitters();
    for (auto* component : std::initializer_list<juce::Component*>{&transportBar_,
                                                                   &patternList_,
                                                                   &songTimeline_,
                                                                   &pianoRollView_,
                                                                   &metronomePanel_,
                                                                   &noteInspector_,
                                                                   &mixPanel_,
                                                                   &leftSplitter_,
                                                                   &rightSplitter_,
                                                                   &songSplitter_,
                                                                   &status_,
                                                                   &editStatus_})
        addAndMakeVisible(component);
    deviceManager_.addChangeListener(this);
    updateStatus();
    startTimerHz(kTimerHz);
    setSize(1440, 960);
}

/// The splitters change the pattern column, the right column and the height of the song timeline. Each
/// pane keeps the minimum of the others free (AP14).
void MainComponent::setUpSplitters()
{
    leftSplitter_.setDragHandlers(
        [this] { patternColumn_.beginDrag(bodyArea().getWidth(), kBesidePatternColumn); },
        [this](int offset)
        {
            patternColumn_.dragTo(offset, bodyArea().getWidth(), kBesidePatternColumn);
            resized();
        });
    // The splitter is left of the right column: dragging to the right makes the column smaller.
    rightSplitter_.setDragHandlers(
        [this]
        { rightColumn_.beginDrag(bodyArea().getRight() - leftSplitter_.getRight(), kBesideRightColumn); },
        [this](int offset)
        {
            rightColumn_.dragTo(
                -offset, bodyArea().getRight() - leftSplitter_.getRight(), kBesideRightColumn);
            resized();
        });
    songSplitter_.setDragHandlers([this] { songHeight_.beginDrag(bodyArea().getHeight(), kBelowSong); },
                                  [this](int offset)
                                  {
                                      songHeight_.dragTo(offset, bodyArea().getHeight(), kBelowSong);
                                      resized();
                                  });
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager_.removeChangeListener(this);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(theme::kGap);
    for (const auto& panel : panels_)
    {
        g.setColour(theme::kPanel);
        g.fillRect(panel);
        g.setColour(theme::kOutline.withAlpha(0.5F));
        g.drawRect(panel);
    }
    auto leds = ledArea_;
    paintLed(g, leds.removeFromLeft(kLedWidth), "MIDI In", inputLeds_.isMidiLedOn());
    paintLed(g, leds.removeFromLeft(kLedWidth), "Tastatur", inputLeds_.isKeyboardLedOn());
}

juce::Rectangle<int> MainComponent::bodyArea() const
{
    return getLocalBounds()
        .reduced(kGap)
        .withTrimmedTop(kTopBarHeight + kGap)
        .withTrimmedBottom(kStatusHeight);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(kGap);
    auto statusRow = area.removeFromBottom(kStatusHeight).reduced(kPadding, 0);
    editStatus_.setBounds(statusRow.removeFromRight(320));
    status_.setBounds(statusRow);

    // Top: one row of controls, the input LEDs in the right corner.
    topBar_ = area.removeFromTop(kTopBarHeight);
    auto top = topBar_.reduced(kPadding, 0);
    ledArea_ = top.removeFromRight(2 * kLedWidth);
    transportBar_.setBounds(top.withSizeKeepingCentre(top.getWidth(), TransportBar::kPreferredHeight));
    area.removeFromTop(kGap);
    layOutColumns(area);
    repaint();
}

/// Three columns: patterns, editing, settings (AP14).
void MainComponent::layOutColumns(juce::Rectangle<int> area)
{
    auto left = area.removeFromLeft(patternColumn_.size(area.getWidth(), kBesidePatternColumn));
    leftSplitter_.setBounds(area.removeFromLeft(Splitter::kThickness));
    auto right = area.removeFromRight(rightColumn_.size(area.getWidth(), kBesideRightColumn));
    rightSplitter_.setBounds(area.removeFromRight(Splitter::kThickness));

    const int songHeight = songHeight_.size(area.getHeight(), kBelowSong);
    const auto song = area.removeFromTop(songHeight);
    songSplitter_.setBounds(area.removeFromTop(Splitter::kThickness));
    // The right column follows the song splitter: the note inspector starts level with the piano roll.
    const auto metronome = right.removeFromTop(songHeight);
    right.removeFromTop(Splitter::kThickness);

    panels_ = {topBar_, left, song, area, metronome, right};
    patternList_.setBounds(left.reduced(1));
    songTimeline_.setBounds(song.reduced(1));
    pianoRollView_.setBounds(area.reduced(1));
    metronomePanel_.setBounds(metronome.reduced(kPadding, kPadding / 2));
    auto inspector = right.reduced(kPadding, kPadding / 2);
    noteInspector_.setBounds(inspector.removeFromTop(kNoteInspectorHeight));
    inspector.removeFromTop(kPadding);
    mixPanel_.setBounds(inspector.removeFromTop(MixPanel::kPreferredHeight));
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    updateStatus();
}

void MainComponent::timerCallback()
{
    inputLeds_.tick();
    repaint(ledArea_);
    editStatus_.setText(juce::String::fromUTF8(pianoRoll_.statusText().c_str()), juce::dontSendNotification);
    // Dropouts and a hanging device change without a change message (Q-03, Q-09).
    if (++statusTicks_ >= kTimerHz)
    {
        statusTicks_ = 0;
        updateStatus();
    }
}

void MainComponent::updateStatus()
{
    status_.setText(
        juce::String::fromUTF8(ui::audioStatusText(deviceInfo(deviceManager_, deviceSupervisor_)).c_str()),
        juce::dontSendNotification);
}

void MainComponent::paintLed(juce::Graphics& g,
                             juce::Rectangle<int> area,
                             const juce::String& label,
                             bool on) const
{
    const auto led = area.removeFromLeft(kLedSize + 6).withSizeKeepingCentre(kLedSize, kLedSize).toFloat();
    g.setColour(on ? juce::Colours::limegreen : juce::Colours::darkgrey);
    g.fillEllipse(led);
    g.setColour(getLookAndFeel().findColour(juce::Label::textColourId));
    g.drawText(label, area, juce::Justification::centredLeft);
}

} // namespace drumprog::app
