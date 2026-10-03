#include "engine/VoicePool.h"

#include <algorithm>
#include <cmath>

namespace drumprog::engine
{

void VoicePool::prepare(double sampleRate) noexcept
{
    fadeSamples_ = std::max(1, static_cast<int>(std::lround(sampleRate * kFadeSeconds)));
}

void VoicePool::trigger(const VoiceStart& params) noexcept
{
    if (params.sample == nullptr)
        return;
    choke(params.chokeGroup);
    if (playingVoiceCount() >= kMaxVoices)
        oldestPlayingVoice().release(fadeSamples_);
    freeVoice().start(params, nextStartOrder_++);
}

void VoicePool::choke(int chokeGroup) noexcept
{
    if (chokeGroup <= 0)
        return;
    for (auto& voice : voices_)
        if (voice.isActive() && voice.chokeGroup() == chokeGroup)
            voice.release(fadeSamples_);
}

void VoicePool::stopAll() noexcept
{
    for (auto& voice : voices_)
        voice.stop();
}

void VoicePool::render(float* const* outputs, int numOutputs, int numSamples) noexcept
{
    for (auto& voice : voices_)
        if (voice.isActive())
            voice.render(outputs, numOutputs, numSamples);
}

int VoicePool::playingVoiceCount() const noexcept
{
    return static_cast<int>(std::count_if(voices_.begin(),
                                          voices_.end(),
                                          [](const Voice& voice)
                                          { return voice.isActive() && !voice.isReleasing(); }));
}

int VoicePool::soundingVoiceCount() const noexcept
{
    return static_cast<int>(
        std::count_if(voices_.begin(), voices_.end(), [](const Voice& voice) { return voice.isActive(); }));
}

bool VoicePool::isOwnerInUse(const void* owner) const noexcept
{
    return std::any_of(voices_.begin(),
                       voices_.end(),
                       [owner](const Voice& voice) { return voice.isActive() && voice.owner() == owner; });
}

int VoicePool::fadeSamples() const noexcept
{
    return fadeSamples_;
}

Voice& VoicePool::oldestPlayingVoice() noexcept
{
    // Only called with kMaxVoices playing voices, so the minimum is a playing voice.
    return *std::min_element(voices_.begin(),
                             voices_.end(),
                             [](const Voice& a, const Voice& b)
                             {
                                 const bool aPlaying = a.isActive() && !a.isReleasing();
                                 const bool bPlaying = b.isActive() && !b.isReleasing();
                                 return aPlaying != bPlaying ? aPlaying : a.startOrder() < b.startOrder();
                             });
}

Voice& VoicePool::freeVoice() noexcept
{
    auto* const idle =
        std::find_if(voices_.begin(), voices_.end(), [](const Voice& voice) { return !voice.isActive(); });
    if (idle != voices_.end())
        return *idle;
    // All voices sound: cut the oldest fading voice. At most kMaxVoices play, so one is fading.
    auto& oldest = *std::min_element(
        voices_.begin(),
        voices_.end(),
        [](const Voice& a, const Voice& b)
        { return a.isReleasing() != b.isReleasing() ? a.isReleasing() : a.startOrder() < b.startOrder(); });
    oldest.stop();
    return oldest;
}

} // namespace drumprog::engine
