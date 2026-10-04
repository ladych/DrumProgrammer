#pragma once

#include "engine/EngineKit.h"
#include "engine/SpscQueue.h"
#include "engine/TriggerIndicators.h"
#include "engine/VoicePool.h"

#include <atomic>
#include <memory>

namespace drumprog::engine
{

/// Plays the kit on the audio thread. The GUI thread publishes immutable kits
/// and queues triggers, the MIDI thread queues the hits of the drum kit; the
/// audio thread picks them up at the start of a block. Each producer thread has
/// its own lock-free single-producer queue.
/// A replaced kit stays alive until no voice plays its samples any more and is
/// then deleted on the GUI thread, never in the callback (Q-04).
class SampleEngine
{
public:
    static constexpr int kPreviewVelocity = 100; ///< F-SE-10
    static constexpr std::size_t kTriggerQueueSize = 256;

    SampleEngine() = default;
    ~SampleEngine();

    SampleEngine(const SampleEngine&) = delete;
    SampleEngine& operator=(const SampleEngine&) = delete;
    SampleEngine(SampleEngine&&) = delete;
    SampleEngine& operator=(SampleEngine&&) = delete;

    // GUI thread
    void setKit(std::unique_ptr<EngineKit> kit);
    /// Queues a trigger for the next audio block; false if the queue is full.
    bool queueTrigger(int midiNote, int velocity) noexcept;
    /// Same as queueTrigger(), for the MIDI input thread only (F-IN-03).
    bool queueMidiTrigger(int midiNote, int velocity) noexcept;
    bool preview(int midiNote) noexcept;
    /// Deletes kits the audio thread no longer uses. Call regularly, e.g. from a GUI timer.
    void collectGarbage();
    [[nodiscard]] const TriggerIndicators& indicators() const noexcept;

    // Audio thread
    /// Call before playback starts and whenever the sample rate changes.
    void prepare(double sampleRate) noexcept;
    /// Starts a note at a sample position inside the current block (sequencer, AP4).
    void trigger(int midiNote, int velocity, int sampleOffset) noexcept;
    /// Adds the kit's voices to the outputs (the caller clears them first).
    void render(float* const* outputs, int numOutputs, int numSamples) noexcept;

private:
    struct TriggerEvent
    {
        int midiNote = 0;
        int velocity = 0;
    };

    void playQueuedTriggers() noexcept;
    void pickUpPendingKit() noexcept;
    void retireDrainingKit() noexcept;

    VoicePool voices_;
    TriggerIndicators indicators_;
    SpscQueue<TriggerEvent, kTriggerQueueSize> triggers_;     ///< GUI thread -> audio thread
    SpscQueue<TriggerEvent, kTriggerQueueSize> midiTriggers_; ///< MIDI thread -> audio thread
    std::atomic<EngineKit*> pendingKit_{nullptr};
    std::atomic<EngineKit*> retiredKit_{nullptr};
    EngineKit* currentKit_ = nullptr;  ///< audio thread only
    EngineKit* drainingKit_ = nullptr; ///< audio thread only
};

} // namespace drumprog::engine
