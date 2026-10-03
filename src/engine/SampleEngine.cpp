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
    const EngineSlot* slot = currentKit_ != nullptr ? currentKit_->slotForNote(midiNote) : nullptr;
    if (slot == nullptr || velocity <= 0)
        return;
    indicators_.signal(slot->slotIndex);
    voices_.trigger(VoiceStart{.sample = slot->sample.get(),
                               .gain = slot->gain * velocityToGain(velocity),
                               .playbackRate = slot->playbackRate,
                               .chokeGroup = slot->chokeGroup,
                               .startOffset = sampleOffset,
                               .owner = currentKit_});
}

void SampleEngine::render(float* const* outputs, int numOutputs, int numSamples) noexcept
{
    retireDrainingKit();
    pickUpPendingKit();

    TriggerEvent event;
    while (triggers_.pop(event))
        trigger(event.midiNote, event.velocity, 0);

    voices_.render(outputs, numOutputs, numSamples);
    retireDrainingKit();
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
