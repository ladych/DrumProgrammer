#include "engine/PlaybackRenderer.h"

namespace drumprog::engine
{

PlaybackRenderer::PlaybackRenderer(Sequencer& sequencer, SampleEngine& engine, Metronome& metronome)
    : sequencer_(sequencer), engine_(engine), metronome_(metronome)
{
}

void PlaybackRenderer::prepare(double sampleRate) noexcept
{
    sequencer_.prepare(sampleRate);
    engine_.prepare(sampleRate);
    metronome_.prepare(sampleRate);
}

void PlaybackRenderer::render(const ProjectSnapshot* snapshot,
                              float* const* outputs,
                              int numOutputs,
                              int numSamples,
                              double blockTimeSeconds) noexcept
{
    sequencer_.process(snapshot, numSamples, block_);
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
    engine_.render(outputs, numOutputs, numSamples);
    metronome_.render(outputs, numOutputs, numSamples);
    sequencer_.record(engine_.liveHits(), blockTimeSeconds);
}

} // namespace drumprog::engine
