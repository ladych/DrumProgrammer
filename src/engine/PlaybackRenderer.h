#pragma once

#include "engine/Metronome.h"
#include "engine/ProjectSnapshot.h"
#include "engine/SampleEngine.h"
#include "engine/Sequencer.h"

namespace drumprog::engine
{

/// Everything the audio callback renders per block (Pflichtenheft chapter 3): the sequencer reads
/// the project snapshot of the block and places notes and metronome clicks on their samples, the
/// SampleEngine plays them together with the live hits, and the live hits go back to the sequencer
/// for the recording. Audio thread only, no allocation (Q-04).
class PlaybackRenderer
{
public:
    PlaybackRenderer(Sequencer& sequencer, SampleEngine& engine, Metronome& metronome);

    void prepare(double sampleRate) noexcept;
    /// Adds the block to the outputs. snapshot is the current one of the SnapshotExchange (null before
    /// the first), blockTimeSeconds the Clock time at the start of the callback.
    void render(const ProjectSnapshot* snapshot,
                float* const* outputs,
                int numOutputs,
                int numSamples,
                double blockTimeSeconds) noexcept;

private:
    Sequencer& sequencer_;
    SampleEngine& engine_;
    Metronome& metronome_;
    SequencerBlock block_;
};

} // namespace drumprog::engine
