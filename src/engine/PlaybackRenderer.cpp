#include "engine/PlaybackRenderer.h"

#include <algorithm>

namespace drumprog::engine
{

PlaybackRenderer::PlaybackRenderer(Sequencer& sequencer,
                                   SampleEngine& engine,
                                   Metronome& metronome,
                                   BackingTrackPlayer& backing)
    : sequencer_(sequencer), engine_(engine), metronome_(metronome), backing_(backing)
{
}

void PlaybackRenderer::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    sequencer_.prepare(sampleRate);
    engine_.prepare(sampleRate);
    metronome_.prepare(sampleRate);
    backing_.prepare(sampleRate);
}

void PlaybackRenderer::render(const ProjectSnapshot* snapshot,
                              float* const* outputs,
                              int numOutputs,
                              int numSamples,
                              double blockTimeSeconds) noexcept
{
    const int channels = std::min(numOutputs, kMaxChannels);
    if (channels <= 0)
        return;
    for (int done = 0; done < numSamples; done += kMaxStep)
    {
        const Channels step{outputs[0] + done, outputs[channels - 1] + done};
        renderStep(snapshot,
                   step,
                   channels,
                   std::min(kMaxStep, numSamples - done),
                   blockTimeSeconds + static_cast<double>(done) / sampleRate_);
    }
}

void PlaybackRenderer::renderStep(const ProjectSnapshot* snapshot,
                                  const Channels& outputs,
                                  int numOutputs,
                                  int numSamples,
                                  double timeSeconds) noexcept
{
    backing_.update();
    const std::int64_t backingEnd =
        snapshot != nullptr ? backing_.endSample(snapshot->backingTrack.offsetSamples) : 0;
    sequencer_.process(snapshot, numSamples, block_, backingEnd);
    for (std::size_t index = 0; index < block_.numNotes; ++index)
    {
        const auto& note = block_.notes.at(index);
        engine_.triggerSlot(note.slotIndex, note.velocity, note.sampleOffset);
    }
    for (std::size_t index = 0; index < block_.numClicks; ++index)
    {
        const auto& click = block_.clicks.at(index);
        metronome_.click(click.accent, click.sampleOffset);
    }
    const MixSnapshot mix = snapshot != nullptr ? snapshot->mix : MixSnapshot{};
    renderDrums(outputs, numOutputs, numSamples, static_cast<float>(mix.drumsGain * mix.masterGain));
    renderBacking(snapshot, outputs, numOutputs);
    metronome_.render(outputs.data(), numOutputs, numSamples);
    sequencer_.record(engine_.liveHits(), timeSeconds);
}

void PlaybackRenderer::renderDrums(const Channels& outputs,
                                   int numOutputs,
                                   int numSamples,
                                   float gain) noexcept
{
    Channels drums{};
    for (std::size_t channel = 0; channel < drums.size(); ++channel)
    {
        std::fill_n(drums_.at(channel).begin(), numSamples, 0.0F);
        drums.at(channel) = drums_.at(channel).data();
    }
    engine_.render(drums.data(), numOutputs, numSamples);
    for (std::size_t channel = 0; channel < static_cast<std::size_t>(numOutputs); ++channel)
        for (std::size_t index = 0; index < static_cast<std::size_t>(numSamples); ++index)
            outputs.at(channel)[index] += gain * drums.at(channel)[index];
}

void PlaybackRenderer::renderBacking(const ProjectSnapshot* snapshot,
                                     const Channels& outputs,
                                     int numOutputs) noexcept
{
    if (snapshot == nullptr)
        return;
    const auto& mix = snapshot->mix;
    const auto gain = static_cast<float>(snapshot->backingTrack.gain * mix.backingGain * mix.masterGain);
    for (std::size_t index = 0; index < block_.numSongSpans; ++index)
    {
        const auto& span = block_.songSpans.at(index);
        const Channels target{outputs[0] + span.sampleOffset, outputs[1] + span.sampleOffset};
        backing_.render(target.data(),
                        numOutputs,
                        span.numSamples,
                        span.songSample,
                        snapshot->backingTrack.offsetSamples,
                        gain);
    }
}

} // namespace drumprog::engine
