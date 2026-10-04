#include "ui/KitPresenter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr int kMaxMidiNote = 127;

} // namespace

KitPresenter::KitPresenter(engine::KitDescription kit,
                           engine::KitBuilder& builder,
                           engine::SampleEngine& engine)
    : kit_(std::move(kit)), builder_(builder), engine_(engine)
{
    publish();
}

void KitPresenter::setDeviceSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    publish();
}

const engine::KitDescription& KitPresenter::kit() const noexcept
{
    return kit_;
}

std::vector<int> KitPresenter::visibleSlots(bool showAll) const
{
    std::vector<int> visible;
    for (std::size_t i = 0; i < kit_.size(); ++i)
        if (showAll || kit_[i].coreSlot)
            visible.push_back(static_cast<int>(i));
    return visible;
}

std::string KitPresenter::sampleLabel(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(kit_.size()))
        return {};
    const auto& file = kit_[static_cast<std::size_t>(slotIndex)].sampleFile;
    if (file.empty())
        return "kein Sample";
    const auto name = file.filename().u8string();
    return {name.begin(), name.end()};
}

std::string KitPresenter::noteLabel(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(kit_.size()))
        return {};
    const auto& slot = kit_[static_cast<std::size_t>(slotIndex)];
    auto label = std::to_string(slot.midiNote);
    if (slot.usesGmNote())
        label += " (GM-Default)";
    return label;
}

const std::vector<std::filesystem::path>& KitPresenter::missingSamples() const noexcept
{
    return missing_;
}

void KitPresenter::select(int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < static_cast<int>(kit_.size()))
        selected_ = slotIndex;
}

std::optional<int> KitPresenter::selectedSlot() const noexcept
{
    return selected_;
}

bool KitPresenter::loadSample(const std::filesystem::path& file)
{
    auto* slot = selected();
    if (slot == nullptr)
        return false;
    auto previous = std::exchange(slot->sampleFile, file);
    publish();
    if (std::find(missing_.begin(), missing_.end(), file) == missing_.end())
        return true;
    slot->sampleFile = std::move(previous);
    publish();
    return false;
}

void KitPresenter::setGainDb(float gainDb)
{
    if (auto* slot = selected())
    {
        slot->gain = std::pow(10.0F, std::clamp(gainDb, kMinGainDb, kMaxGainDb) / 20.0F);
        publish();
    }
}

float KitPresenter::gainDb() const
{
    if (!selected_)
        return 0.0F;
    const float gain = kit_[static_cast<std::size_t>(*selected_)].gain;
    return gain > 0.0F ? std::max(20.0F * std::log10(gain), kMinGainDb) : kMinGainDb;
}

void KitPresenter::setPitch(int semitones)
{
    if (auto* slot = selected())
    {
        slot->pitchSemitones = std::clamp(
            semitones, -engine::KitBuilder::kMaxPitchSemitones, engine::KitBuilder::kMaxPitchSemitones);
        publish();
    }
}

void KitPresenter::setMidiNote(int midiNote)
{
    if (auto* slot = selected())
    {
        slot->midiNote = std::clamp(midiNote, 0, kMaxMidiNote);
        publish();
    }
}

bool KitPresenter::previewSelected()
{
    const auto* slot = selected();
    return slot != nullptr && engine_.preview(slot->midiNote);
}

void KitPresenter::tick()
{
    engine_.collectGarbage();
    const auto& indicators = engine_.indicators();
    for (std::size_t i = 0; i < seenHits_.size(); ++i)
    {
        const auto hits = indicators.hitCount(static_cast<int>(i));
        auto& ledTicks = ledTicks_.at(i);
        if (hits != seenHits_.at(i))
        {
            seenHits_.at(i) = hits;
            ledTicks = kLedHoldTicks;
        }
        else if (ledTicks > 0)
        {
            --ledTicks;
        }
    }
}

bool KitPresenter::isLedOn(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(ledTicks_.size()))
        return false;
    return ledTicks_.at(static_cast<std::size_t>(slotIndex)) > 0;
}

engine::KitSlotDescription* KitPresenter::selected()
{
    return selected_ ? &kit_[static_cast<std::size_t>(*selected_)] : nullptr;
}

void KitPresenter::publish()
{
    auto result = builder_.build(kit_, sampleRate_);
    missing_ = std::move(result.missingSamples);
    engine_.setKit(std::move(result.kit));
}

} // namespace drumprog::ui
