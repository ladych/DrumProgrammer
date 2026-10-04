#pragma once

#include "engine/KitPublisher.h"
#include "engine/SampleEngine.h"
#include "model/Project.h"
#include "model/TreeChangeListener.h"
#include "ui/ActivityLed.h"

#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Logic behind the kit panel, the sample part of the inspector (humble view in app/KitPanel) and the
/// menu "Kit". It edits the active kit: the project's own kit, which is saved with the project, or
/// otherwise the global kit (program setting). Every change is undoable (F-PJ-05); the
/// SnapshotPublisher hands the changed kit to the engine. GUI thread only.
class KitPresenter
{
public:
    static constexpr float kMinGainDb = -60.0F;
    static constexpr float kMaxGainDb = 6.0F;

    KitPresenter(juce::ValueTree project,
                 juce::ValueTree globalKit,
                 juce::UndoManager& undoManager,
                 engine::KitPublisher& kits,
                 engine::SampleEngine& engine);

    [[nodiscard]] int numSlots() const;
    /// Slot indices to show: only core slots unless showAll (F-SE-04).
    [[nodiscard]] std::vector<int> visibleSlots(bool showAll) const;
    /// Empty for indices outside the kit, like the other per-slot getters.
    [[nodiscard]] std::string slotName(int slotIndex) const;
    /// -1 for indices outside the kit.
    [[nodiscard]] int midiNote(int slotIndex) const;
    /// File name of the slot's sample or "kein Sample".
    [[nodiscard]] std::string sampleLabel(int slotIndex) const;
    /// MIDI note, with " (GM-Default)" while not overridden (F-SE-05).
    [[nodiscard]] std::string noteLabel(int slotIndex) const;
    [[nodiscard]] const std::vector<std::filesystem::path>& missingSamples() const noexcept;

    /// Ignores indices outside the kit.
    void select(int slotIndex);
    [[nodiscard]] std::optional<int> selectedSlot() const noexcept;

    // Edit the selected slot; without a selection these do nothing. Repeated changes of the same
    // value of the same slot, e.g. while dragging a slider, form one undo step.
    /// Returns false and keeps the previous sample if the file cannot be loaded.
    bool loadSample(const std::filesystem::path& file);
    void setGainDb(float gainDb);
    [[nodiscard]] float gainDb() const;
    void setPitch(int semitones);
    [[nodiscard]] int pitch() const;
    void setMidiNote(int midiNote);
    bool previewSelected();

    // Kit source. Each of these is one undo step and does nothing where it makes no sense.
    /// True if the project has its own kit, which overrides the global kit.
    [[nodiscard]] bool usesProjectKit() const;
    /// "Projekt-Kit" or "Programm-Kit", for the kit panel.
    [[nodiscard]] std::string kitSourceLabel() const;
    /// Copies the global kit into the project; the project then keeps its sounds when the global kit
    /// changes.
    void copyGlobalKitToProject();
    /// Removes the project's own kit, so the project plays with the global kit again.
    void useGlobalKit();
    /// Makes the project's own kit the global kit for all projects without their own kit.
    void copyProjectKitToGlobal();

    /// Call from the GUI timer: updates LED states and frees retired kits.
    void tick();
    [[nodiscard]] bool isLedOn(int slotIndex) const;
    /// Changes whenever the project or the global kit changed, also by undo or loading; the view refreshes on
    /// change.
    [[nodiscard]] std::uint32_t changeCount() const noexcept { return changeCount_; }

private:
    [[nodiscard]] model::Project project() const;
    [[nodiscard]] model::Kit activeKit() const;
    [[nodiscard]] std::optional<model::SampleSlot> slotAt(int slotIndex) const;
    [[nodiscard]] std::optional<model::SampleSlot> selected() const;
    void beginEdit(const model::SampleSlot& slot, const std::string& what);

    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    juce::UndoManager& undoManager_;
    engine::KitPublisher& kits_;
    engine::SampleEngine& engine_;
    std::optional<int> selected_;
    std::array<ActivityLed, engine::TriggerIndicators::kMaxSlots> leds_{};
    std::uint32_t changeCount_ = 0;
    model::TreeChangeListener projectListener_;
    model::TreeChangeListener globalKitListener_;
};

} // namespace drumprog::ui
