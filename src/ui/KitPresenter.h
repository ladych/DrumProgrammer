#pragma once

#include "engine/KitBuilder.h"
#include "engine/KitDescription.h"
#include "engine/SampleEngine.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Logic behind the kit panel and the sample part of the inspector (humble
/// view in app/KitPanel). Every change rebuilds the engine kit and publishes it.
/// GUI thread only.
class KitPresenter
{
public:
    static constexpr int kLedHoldTicks = 4; ///< about 130 ms at a 30 Hz GUI timer
    static constexpr float kMinGainDb = -60.0F;
    static constexpr float kMaxGainDb = 6.0F;

    KitPresenter(engine::KitDescription kit, engine::KitBuilder& builder, engine::SampleEngine& engine);

    void setDeviceSampleRate(double sampleRate);

    [[nodiscard]] const engine::KitDescription& kit() const noexcept;
    /// Slot indices to show: only core slots unless showAll (F-SE-04).
    [[nodiscard]] std::vector<int> visibleSlots(bool showAll) const;
    /// File name of the slot's sample or "kein Sample".
    [[nodiscard]] std::string sampleLabel(int slotIndex) const;
    /// MIDI note, with " (GM-Default)" while not overridden (F-SE-05).
    [[nodiscard]] std::string noteLabel(int slotIndex) const;
    [[nodiscard]] const std::vector<std::filesystem::path>& missingSamples() const noexcept;

    /// Ignores indices outside the kit.
    void select(int slotIndex);
    [[nodiscard]] std::optional<int> selectedSlot() const noexcept;

    // Edit the selected slot; without a selection these do nothing.
    /// Returns false and keeps the previous sample if the file cannot be loaded.
    bool loadSample(const std::filesystem::path& file);
    void setGainDb(float gainDb);
    [[nodiscard]] float gainDb() const;
    void setPitch(int semitones);
    void setMidiNote(int midiNote);
    bool previewSelected();

    /// Call from the GUI timer: updates LED states and frees retired kits.
    void tick();
    [[nodiscard]] bool isLedOn(int slotIndex) const;

private:
    [[nodiscard]] engine::KitSlotDescription* selected();
    void publish();

    engine::KitDescription kit_;
    engine::KitBuilder& builder_;
    engine::SampleEngine& engine_;
    double sampleRate_ = 48000.0;
    std::optional<int> selected_;
    std::vector<std::filesystem::path> missing_;
    std::array<std::uint32_t, engine::TriggerIndicators::kMaxSlots> seenHits_{};
    std::array<int, engine::TriggerIndicators::kMaxSlots> ledTicks_{};
};

} // namespace drumprog::ui
