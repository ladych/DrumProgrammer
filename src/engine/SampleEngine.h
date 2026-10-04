#pragma once

#include "engine/EngineKit.h"
#include "engine/LiveHit.h"
#include "engine/SpscQueue.h"
#include "engine/TriggerIndicators.h"
#include "engine/VoicePool.h"

#include <array>
#include <atomic>
#include <memory>
#include <span>

namespace drumprog::engine
{

/// Plays the kit on the audio thread. The GUI thread publishes immutable kits
/// and queues triggers, the MIDI thread queues the hits of the drum kit; the
/// audio thread picks them up at the start of a block. Each producer thread has
/// its own lock-free single-producer queue. Live hits keep the time they were
/// played, so the sequencer can record them (F-IN-07).
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

    static constexpr std::size_t kMaxLiveHitsPerBlock = 2 * kTriggerQueueSize;

    // GUI thread
    void setKit(std::unique_ptr<EngineKit> kit);
    /// Queues a trigger for the next audio block that is never recorded, e.g. a preview; false if the
    /// queue is full.
    bool queueTrigger(int midiNote, int velocity) noexcept;
    /// Queues a hit of the computer keyboard played at timeSeconds (F-IN-01); false if the queue is full.
    bool queueLiveTrigger(int midiNote, int velocity, double timeSeconds) noexcept;
    /// Same as queueLiveTrigger(), for the MIDI input thread only (F-IN-03).
    bool queueMidiTrigger(int midiNote, int velocity, double timeSeconds) noexcept;
    bool preview(int midiNote) noexcept;
    /// Deletes kits the audio thread no longer uses. Call regularly, e.g. from a GUI timer.
    void collectGarbage();
    [[nodiscard]] const TriggerIndicators& indicators() const noexcept;

    // Audio thread
    /// Call before playback starts and whenever the sample rate changes.
    void prepare(double sampleRate) noexcept;
    /// Starts the slot of a MIDI note at a sample position inside the current block.
    void trigger(int midiNote, int velocity, int sampleOffset) noexcept;
    /// Starts a slot by its index in the kit, as the sequencer does (F-TR-06).
    void triggerSlot(int slotIndex, int velocity, int sampleOffset) noexcept;
    /// Adds the kit's voices to the outputs (the caller clears them first).
    void render(float* const* outputs, int numOutputs, int numSamples) noexcept;
    /// Live hits the last render() played, valid until the next one.
    [[nodiscard]] std::span<const LiveHit> liveHits() const noexcept;

private:
    struct TriggerEvent
    {
        int midiNote = 0;
        int velocity = 0;
        double timeSeconds = 0.0;
        bool live = false;
    };

    bool play(const EngineSlot* slot, int velocity, int sampleOffset) noexcept;
    void playQueuedTriggers() noexcept;
    void playQueued(const TriggerEvent& event) noexcept;
    void pickUpPendingKit() noexcept;
    void retireDrainingKit() noexcept;

    VoicePool voices_;
    TriggerIndicators indicators_;
    SpscQueue<TriggerEvent, kTriggerQueueSize> triggers_;     ///< GUI thread -> audio thread
    SpscQueue<TriggerEvent, kTriggerQueueSize> midiTriggers_; ///< MIDI thread -> audio thread
    std::atomic<EngineKit*> pendingKit_{nullptr};
    std::atomic<EngineKit*> retiredKit_{nullptr};
    EngineKit* currentKit_ = nullptr;                      ///< audio thread only
    EngineKit* drainingKit_ = nullptr;                     ///< audio thread only
    std::array<LiveHit, kMaxLiveHitsPerBlock> liveHits_{}; ///< audio thread only
    std::size_t numLiveHits_ = 0;
};

} // namespace drumprog::engine
