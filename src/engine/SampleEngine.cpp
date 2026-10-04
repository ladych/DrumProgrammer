#include "engine/SampleEngine.h"

#include "engine/VelocityCurve.h"

namespace drumprog::engine
{

SampleEngine::~SampleEngine()
{
    delete pendingKit_.load();
    delete retiredKit_.load();
    delete drainingKit_;
    delete currentKit_;
}

void SampleEngine::setKit(std::unique_ptr<EngineKit> kit)
{
    // A kit the audio thread never picked up is still owned by this thread.
    delete pendingKit_.exchange(kit.release(), std::memory_order_acq_rel);
}

bool SampleEngine::queueTrigger(int midiNote, int velocity) noexcept
{
    return triggers_.push(TriggerEvent{midiNote, velocity});
}

bool SampleEngine::queueLiveTrigger(int midiNote, int velocity, double timeSeconds) noexcept
{
    return triggers_.push(TriggerEvent{midiNote, velocity, timeSeconds, true});
}

bool SampleEngine::queueMidiTrigger(int midiNote, int velocity, double timeSeconds) noexcept
{
    return midiTriggers_.push(TriggerEvent{midiNote, velocity, timeSeconds, true});
}

bool SampleEngine::preview(int midiNote) noexcept
{
    return queueTrigger(midiNote, kPreviewVelocity);
}

void SampleEngine::collectGarbage()
{
    delete retiredKit_.exchange(nullptr, std::memory_order_acq_rel);
}

const TriggerIndicators& SampleEngine::indicators() const noexcept
{
    return indicators_;
}

void SampleEngine::prepare(double sampleRate) noexcept
{
    voices_.prepare(sampleRate);
}

void SampleEngine::trigger(int midiNote, int velocity, int sampleOffset) noexcept
{
    play(currentKit_ != nullptr ? currentKit_->slotForNote(midiNote) : nullptr, velocity, sampleOffset);
}

void SampleEngine::triggerSlot(int slotIndex, int velocity, int sampleOffset) noexcept
{
    play(currentKit_ != nullptr ? currentKit_->slotAt(slotIndex) : nullptr, velocity, sampleOffset);
}

std::span<const LiveHit> SampleEngine::liveHits() const noexcept
{
    return {liveHits_.data(), numLiveHits_};
}

bool SampleEngine::play(const EngineSlot* slot, int velocity, int sampleOffset) noexcept
{
    if (slot == nullptr || velocity <= 0)
        return false;
    indicators_.signal(slot->slotIndex);
    voices_.trigger(VoiceStart{.sample = slot->sample.get(),
                               .gain = slot->gain * velocityToGain(velocity),
                               .playbackRate = slot->playbackRate,
                               .chokeGroup = slot->chokeGroup,
                               .startOffset = sampleOffset,
                               .owner = currentKit_});
    return true;
}

void SampleEngine::render(float* const* outputs, int numOutputs, int numSamples) noexcept
{
    retireDrainingKit();
    pickUpPendingKit();

    playQueuedTriggers();
    voices_.render(outputs, numOutputs, numSamples);
    retireDrainingKit();
}

void SampleEngine::playQueuedTriggers() noexcept
{
    numLiveHits_ = 0;
    // At most one queue length each, so a producer that keeps pushing cannot hold up the block.
    TriggerEvent event;
    for (std::size_t count = 0; count < kTriggerQueueSize && triggers_.pop(event); ++count)
        playQueued(event);
    for (std::size_t count = 0; count < kTriggerQueueSize && midiTriggers_.pop(event); ++count)
        playQueued(event);
}

void SampleEngine::playQueued(const TriggerEvent& event) noexcept
{
    const EngineSlot* slot = currentKit_ != nullptr ? currentKit_->slotForNote(event.midiNote) : nullptr;
    if (!play(slot, event.velocity, 0) || !event.live)
        return;
    // At most kTriggerQueueSize events of each queue per block, so there is always room.
    liveHits_.at(numLiveHits_++) = {slot->slotIndex, event.velocity, event.timeSeconds};
}

void SampleEngine::pickUpPendingKit() noexcept
{
    // Only one replaced kit at a time; a newer one waits until it is retired.
    if (drainingKit_ != nullptr)
        return;
    if (EngineKit* next = pendingKit_.exchange(nullptr, std::memory_order_acq_rel))
    {
        drainingKit_ = currentKit_;
        currentKit_ = next;
    }
}

void SampleEngine::retireDrainingKit() noexcept
{
    if (drainingKit_ == nullptr || voices_.isOwnerInUse(drainingKit_))
        return;
    // The GUI thread clears the retire slot; until it does, keep draining.
    if (retiredKit_.load(std::memory_order_acquire) != nullptr)
        return;
    retiredKit_.store(drainingKit_, std::memory_order_release);
    drainingKit_ = nullptr;
}

} // namespace drumprog::engine
