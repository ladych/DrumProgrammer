#pragma once

#include "engine/BackingTrackPlayer.h"
#include "engine/Metronome.h"
#include "engine/ProjectSnapshot.h"
#include "engine/SampleEngine.h"
#include "engine/Sequencer.h"

#include <array>

namespace drumprog::engine
{

/// Everything the audio callback renders per block (Pflichtenheft chapter 3): the sequencer reads
/// the project snapshot of the block and places notes and metronome clicks on their samples, the
/// SampleEngine plays them together with the live hits, the backing track plays along in the song mode,
/// and the live hits go back to the sequencer for the recording. Drums and backing track get the gains
/// of the mix (F-BT-06); the metronome keeps its own level. Audio thread only, no allocation (Q-04).
class PlaybackRenderer
{
public:
    /// Long driver blocks are rendered in steps of at most this many samples.
    static constexpr int kMaxStep = 512;
    static constexpr int kMaxChannels = 2;

    PlaybackRenderer(Sequencer& sequencer,
                     SampleEngine& engine,
                     Metronome& metronome,
                     BackingTrackPlayer& backing);

    void prepare(double sampleRate) noexcept;
    /// Adds the block to the first two outputs. snapshot is the current one of the SnapshotExchange (null
    /// before the first), blockTimeSeconds the Clock time at the start of the callback.
    void render(const ProjectSnapshot* snapshot,
                float* const* outputs,
                int numOutputs,
                int numSamples,
                double blockTimeSeconds) noexcept;

private:
    using Channels = std::array<float*, kMaxChannels>;

    void renderStep(const ProjectSnapshot* snapshot,
                    const Channels& outputs,
                    int numOutputs,
                    int numSamples,
                    double timeSeconds) noexcept;
    void renderDrums(const Channels& outputs, int numOutputs, int numSamples, float gain) noexcept;
    void renderBacking(const ProjectSnapshot* snapshot, const Channels& outputs, int numOutputs) noexcept;

    Sequencer& sequencer_;
    SampleEngine& engine_;
    Metronome& metronome_;
    BackingTrackPlayer& backing_;
    SequencerBlock block_;
    double sampleRate_ = 48000.0;
    std::array<std::array<float, kMaxStep>, kMaxChannels> drums_{};
};

} // namespace drumprog::engine
