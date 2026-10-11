#include "ui/KitPresenter.h"

#include "i18n/Translate.h"
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
                           juce::ValueTree globalKit,
                           juce::UndoManager& undoManager,
                           engine::KitPublisher& kits,
                           engine::SampleEngine& engine)
    : project_(std::move(project)), globalKit_(std::move(globalKit)), undoManager_(undoManager), kits_(kits),
      engine_(engine), projectListener_(project_, [this] { ++changeCount_; }),
      globalKitListener_(globalKit_, [this] { ++changeCount_; })
{
}

int KitPresenter::numSlots() const
{
    return activeKit().numSlots();
}

std::vector<int> KitPresenter::visibleSlots(bool showAll) const
{
    std::vector<int> visible;
    const auto kit = activeKit();
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
        return i18n::tr("no sample");
    return io::utf8FromPath(io::pathFromUtf8(slot->filePath()).filename());
}

std::string KitPresenter::noteLabel(int slotIndex) const
{
    const auto slot = slotAt(slotIndex);
    if (!slot)
        return {};
    auto label = std::to_string(slot->midiNote());
    if (slot->midiNote() == slot->gmNote())
        label = i18n::tr("%1 (GM default)", label);
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
    beginEdit(*slot, i18n::marked("Sample %1"));
    slot->setFilePath(io::utf8FromPath(file));
    return true;
}

void KitPresenter::setGainDb(float gainDb)
{
    if (auto slot = selected())
    {
        beginEdit(*slot, i18n::marked("Volume %1"));
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
        beginEdit(*slot, i18n::marked("Pitch %1"));
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
        beginEdit(*slot, i18n::marked("MIDI note %1"));
        slot->setMidiNote(std::clamp(midiNote, 0, kMaxMidiNote));
    }
}

bool KitPresenter::previewSelected()
{
    const auto slot = selected();
    return slot && engine_.preview(slot->midiNote());
}

bool KitPresenter::usesProjectKit() const
{
    return project().hasOwnKit();
}

std::string KitPresenter::kitSourceLabel() const
{
    return usesProjectKit() ? i18n::tr("Project kit") : i18n::tr("Program kit");
}

void KitPresenter::copyGlobalKitToProject()
{
    if (usesProjectKit())
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Copy kit into the project"));
    project().setOwnKit(globalKit_);
}

void KitPresenter::useGlobalKit()
{
    if (!usesProjectKit())
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Use program kit"));
    project().removeOwnKit();
}

void KitPresenter::copyProjectKitToGlobal()
{
    if (!usesProjectKit())
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Make project kit the program kit"));
    globalKit_.copyPropertiesAndChildrenFrom(project().kit().tree(), &undoManager_);
}

void KitPresenter::resetToDefaultKit(const juce::ValueTree& defaultKit)
{
    undoManager_.beginNewTransaction(i18n::trJuce("Reset kit to default"));
    auto kit = activeKit().tree();
    kit.copyPropertiesAndChildrenFrom(defaultKit, &undoManager_);
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

model::Project KitPresenter::project() const
{
    return {project_, &undoManager_};
}

model::Kit KitPresenter::activeKit() const
{
    return project().activeKit(globalKit_);
}

std::optional<model::SampleSlot> KitPresenter::slotAt(int slotIndex) const
{
    const auto kit = activeKit();
    if (slotIndex < 0 || slotIndex >= kit.numSlots())
        return std::nullopt;
    return kit.slot(slotIndex);
}

std::optional<model::SampleSlot> KitPresenter::selected() const
{
    return selected_ ? slotAt(*selected_) : std::nullopt;
}

void KitPresenter::beginEdit(const model::SampleSlot& slot, const char* what)
{
    const auto name = i18n::trJuce(what, slot.name());
    if (undoManager_.getCurrentTransactionName() != name)
        undoManager_.beginNewTransaction(name);
}

} // namespace drumprog::ui
