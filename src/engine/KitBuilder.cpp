#include "engine/KitBuilder.h"

#include "engine/Downmix.h"
#include "engine/Resampler.h"

#include <algorithm>
#include <cmath>

namespace drumprog::engine
{

double pitchToPlaybackRate(int semitones) noexcept
{
    const int clamped =
        std::clamp(semitones, -KitBuilder::kMaxPitchSemitones, KitBuilder::kMaxPitchSemitones);
    return std::exp2(static_cast<double>(clamped) / 12.0);
}

KitBuilder::KitBuilder(ISampleLoader& loader) : loader_(loader) {}

KitBuildResult KitBuilder::build(const KitDescription& kit, double deviceSampleRate)
{
    if (deviceSampleRate != cachedRate_)
    {
        resampled_.clear();
        cachedRate_ = deviceSampleRate;
    }

    KitBuildResult result;
    std::vector<EngineSlot> slots;
    slots.reserve(kit.size());
    for (const auto& description : kit)
    {
        EngineSlot slot{.slotIndex = static_cast<int>(slots.size()),
                        .midiNote = description.midiNote,
                        .gain = description.gain,
                        .playbackRate = pitchToPlaybackRate(description.pitchSemitones),
                        .chokeGroup = description.chokeGroup};
        if (!description.sampleFile.empty())
        {
            slot.sample = sampleFor(description.sampleFile);
            if (slot.sample == nullptr)
                result.missingSamples.push_back(description.sampleFile);
        }
        slots.push_back(std::move(slot));
    }
    result.kit = std::make_unique<EngineKit>(std::move(slots));
    return result;
}

bool KitBuilder::preload(const std::filesystem::path& file)
{
    return originalFor(file) != nullptr;
}

std::shared_ptr<const SampleBuffer> KitBuilder::sampleFor(const std::filesystem::path& file)
{
    if (const auto found = resampled_.find(file); found != resampled_.end())
        return found->second;

    const auto original = originalFor(file);
    if (original == nullptr)
        return nullptr;
    auto converted = std::make_shared<const SampleBuffer>(resample(*original, cachedRate_));
    resampled_[file] = converted;
    return converted;
}

std::shared_ptr<const SampleBuffer> KitBuilder::originalFor(const std::filesystem::path& file)
{
    if (const auto found = originals_.find(file); found != originals_.end())
        return found->second;

    auto loaded = loader_.load(file);
    if (!loaded)
        return nullptr;
    auto original = std::make_shared<const SampleBuffer>(downmixToStereo(*loaded));
    originals_[file] = original;
    return original;
}

} // namespace drumprog::engine
