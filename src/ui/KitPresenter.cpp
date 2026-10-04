#include "ui/KitPresenter.h"

#include "io/Utf8Path.h"

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

KitPresenter::KitPresenter(juce::ValueTree project,
                           juce::UndoManager& undoManager,
                           engine::KitPublisher& kits,
                           engine::SampleEngine& engine)
    : project_(std::move(project)), undoManager_(undoManager), kits_(kits), engine_(engine),
      listener_(project_, [this] { ++changeCount_; })
{
}

int KitPresenter::numSlots() const
{
    return model::Project{project_, nullptr}.kit().numSlots();
}

std::vector<int> KitPresenter::visibleSlots(bool showAll) const
{
    std::vector<int> visible;
    const auto kit = model::Project{project_, nullptr}.kit();
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const int gmNote = kit.slot(index).gmNote();
        if (showAll || engine::isCoreGmNote(gmNote))
            visible.push_back(index);
    }
    return visible;
}

std::string KitPresenter::slotName(int slotIndex) const
{
    const auto slot = slotAt(slotIndex);
    return slot ? slot->name() : std::string{};
}

int KitPresenter::midiNote(int slotIndex) const
{
    const auto slot = slotAt(slotIndex);
    return slot ? slot->midiNote() : -1;
}

std::string KitPresenter::sampleLabel(int slotIndex) const
{
    const auto slot = slotAt(slotIndex);
    if (!slot)
        return {};
    if (!slot->hasSample())
        return "kein Sample";
    return io::utf8FromPath(io::pathFromUtf8(slot->filePath()).filename());
}

std::string KitPresenter::noteLabel(int slotIndex) const
{
    const auto slot = slotAt(slotIndex);
    if (!slot)
        return {};
    auto label = std::to_string(slot->midiNote());
    if (slot->midiNote() == slot->gmNote())
        label += " (GM-Default)";
    return label;
}

const std::vector<std::filesystem::path>& KitPresenter::missingSamples() const noexcept
{
    return kits_.missingSamples();
}

void KitPresenter::select(int slotIndex)
{
    if (slotAt(slotIndex))
        selected_ = slotIndex;
}

std::optional<int> KitPresenter::selectedSlot() const noexcept
{
    return selected_;
}

bool KitPresenter::loadSample(const std::filesystem::path& file)
{
    auto slot = selected();
    if (!slot || !kits_.canLoad(file))
        return false;
    beginEdit(*slot, "Sample");
    slot->setFilePath(io::utf8FromPath(file));
    return true;
}

void KitPresenter::setGainDb(float gainDb)
{
    if (auto slot = selected())
    {
        beginEdit(*slot, "Lautstärke");
        slot->setGain(std::pow(10.0, std::clamp(gainDb, kMinGainDb, kMaxGainDb) / 20.0));
    }
}

float KitPresenter::gainDb() const
{
    const auto slot = selected();
    if (!slot)
        return 0.0F;
    const double gain = slot->gain();
    const double gainDb = gain > 0.0 ? 20.0 * std::log10(gain) : kMinGainDb;
    return std::max(static_cast<float>(gainDb), kMinGainDb);
}

void KitPresenter::setPitch(int semitones)
{
    if (auto slot = selected())
    {
        beginEdit(*slot, "Pitch");
        slot->setPitch(std::clamp(
            semitones, -engine::KitBuilder::kMaxPitchSemitones, engine::KitBuilder::kMaxPitchSemitones));
    }
}

int KitPresenter::pitch() const
{
    const auto slot = selected();
    return slot ? static_cast<int>(std::lround(slot->pitch())) : 0;
}

void KitPresenter::setMidiNote(int midiNote)
{
    if (auto slot = selected())
    {
        beginEdit(*slot, "MIDI-Note");
        slot->setMidiNote(std::clamp(midiNote, 0, kMaxMidiNote));
    }
}

bool KitPresenter::previewSelected()
{
    const auto slot = selected();
    return slot && engine_.preview(slot->midiNote());
}

void KitPresenter::tick()
{
    engine_.collectGarbage();
    const auto& indicators = engine_.indicators();
    for (std::size_t index = 0; index < leds_.size(); ++index)
        leds_.at(index).update(indicators.hitCount(static_cast<int>(index)));
}

bool KitPresenter::isLedOn(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(leds_.size()))
        return false;
    return leds_.at(static_cast<std::size_t>(slotIndex)).isOn();
}

std::optional<model::SampleSlot> KitPresenter::slotAt(int slotIndex) const
{
    const auto kit = model::Project{project_, &undoManager_}.kit();
    if (slotIndex < 0 || slotIndex >= kit.numSlots())
        return std::nullopt;
    return kit.slot(slotIndex);
}

std::optional<model::SampleSlot> KitPresenter::selected() const
{
    return selected_ ? slotAt(*selected_) : std::nullopt;
}

void KitPresenter::beginEdit(const model::SampleSlot& slot, const std::string& what)
{
    const auto name = juce::String::fromUTF8((what + " " + slot.name()).c_str());
    if (undoManager_.getCurrentTransactionName() != name)
        undoManager_.beginNewTransaction(name);
}

} // namespace drumprog::ui
