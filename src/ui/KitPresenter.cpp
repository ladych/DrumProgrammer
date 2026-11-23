#include "ui/KitPresenter.h"

#include "engine/DrumGizmoMapping.h"
#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/ModelIds.h"
#include "model/ProjectFactory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr int kMaxMidiNote = 127;
constexpr std::uint64_t kBytesPerMb = 1'000'000ULL;
constexpr std::uint64_t kBytesPerGb = 1'000'000'000ULL;

/// "350 MB" below one GB, otherwise "1,2 GB" with the decimal separator of the language.
std::string formatBytes(std::uint64_t bytes)
{
    if (bytes < kBytesPerGb)
        return i18n::tr("%1 MB", std::to_string((bytes + kBytesPerMb / 2) / kBytesPerMb));
    const auto tenths = (bytes + kBytesPerGb / 20) / (kBytesPerGb / 10);
    return i18n::tr("%1 GB",
                    std::to_string(tenths / 10) + i18n::decimalSeparator() + std::to_string(tenths % 10));
}

} // namespace

KitPresenter::KitPresenter(juce::ValueTree project,
                           juce::ValueTree globalKit,
                           juce::UndoManager& undoManager,
                           engine::KitPublisher& kits,
                           engine::SampleEngine& engine,
                           const engine::IDrumGizmoReader& drumGizmoReader)
    : project_(std::move(project)), globalKit_(std::move(globalKit)), undoManager_(undoManager), kits_(kits),
      engine_(engine), drumGizmoReader_(drumGizmoReader),
      projectListener_(project_, [this] { ++changeCount_; }),
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
    if (isDrumGizmoKit())
        return slot->instrument().empty() ? i18n::tr("no instrument") : slot->instrument();
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

engine::DrumGizmoError KitPresenter::loadDrumGizmoKit(const std::filesystem::path& drumkitFile)
{
    auto read = drumGizmoReader_.read(drumkitFile, false);
    if (!read.kit)
        return read.error;
    undoManager_.beginNewTransaction(i18n::trJuce("Load DrumGizmo kit"));
    const auto file = io::utf8FromPath(drumkitFile);
    const bool sameKit = activeKit().drumkitFile() == file;
    const auto newKit = model::ProjectFactory::createDrumGizmoKit(file, engine::mapToGmSlots(*read.kit));
    // Slots first, then the kit file, so the background load starts once, with the new slots.
    auto kit = activeKit().tree();
    kit.removeAllChildren(&undoManager_);
    for (const auto& slot : newKit)
        kit.appendChild(slot.createCopy(), &undoManager_);
    kit.removeProperty(model::ids::sampleMissing, &undoManager_);
    kit.setProperty(model::ids::drumkitFile, newKit[model::ids::drumkitFile], &undoManager_);
    readFile_ = drumkitFile;
    readKit_ = std::move(read);
    // A new kit starts loading with the change of the tree; the same kit is read again, e.g. after its
    // files were fixed.
    if (sameKit)
        kits_.reloadDrumGizmoKit();
    return engine::DrumGizmoError::none;
}

bool KitPresenter::isDrumGizmoKit() const
{
    return activeKit().isDrumGizmo();
}

std::vector<std::string> KitPresenter::instrumentNames() const
{
    std::vector<std::string> names;
    if (!isDrumGizmoKit())
        return names;
    if (const auto& read = drumGizmoKit(); read.kit)
        for (const auto& instrument : read.kit->instruments)
            names.push_back(instrument.name);
    return names;
}

std::string KitPresenter::instrument() const
{
    const auto slot = selected();
    return slot ? slot->instrument() : std::string{};
}

void KitPresenter::setInstrument(const std::string& instrument)
{
    if (auto slot = selected())
    {
        beginEdit(*slot, i18n::marked("Instrument %1"));
        slot->setInstrument(instrument);
    }
}

std::string KitPresenter::drumGizmoStatus() const
{
    if (!isDrumGizmoKit())
        return {};
    return describeDrumGizmoKit(drumGizmoKitName(), kits_.drumGizmoStatus(), kits_.drumGizmoProgress());
}

std::string KitPresenter::describeDrumGizmoKit(const std::string& name,
                                               const engine::DrumGizmoStatus& status,
                                               float progress)
{
    switch (status.state)
    {
    case engine::DrumGizmoStatus::State::loading:
        return i18n::tr(
            "DrumGizmo kit \"%1\" is loading... %2 %", name, std::to_string(std::lround(progress * 100.0F)));
    case engine::DrumGizmoStatus::State::failed:
        return drumGizmoErrorMessage(status.error, status.errorFile);
    case engine::DrumGizmoStatus::State::none:
        return i18n::tr("DrumGizmo kit \"%1\"", name);
    default:
        break;
    }
    auto text = i18n::tr("DrumGizmo kit \"%1\": %2 in RAM", name, formatBytes(status.bytes));
    if (status.missingFiles > 0)
        text += " | " + i18n::tr("%1 audio files missing", std::to_string(status.missingFiles));
    if (status.bytes > kLargeKitBytes)
        text += " | " + i18n::tr("more than 5 GB, the computer may run out of memory");
    return text;
}

bool KitPresenter::isDrumGizmoWarning(const engine::DrumGizmoStatus& status) noexcept
{
    return status.state == engine::DrumGizmoStatus::State::failed || status.missingFiles > 0 ||
           status.bytes > kLargeKitBytes;
}

bool KitPresenter::hasDrumGizmoWarning() const
{
    return isDrumGizmoKit() && isDrumGizmoWarning(kits_.drumGizmoStatus());
}

std::string KitPresenter::drumGizmoErrorMessage(engine::DrumGizmoError error,
                                                const std::filesystem::path& file)
{
    const auto path = io::utf8FromPath(file);
    switch (error)
    {
    case engine::DrumGizmoError::fileMissing:
        return i18n::tr("The DrumGizmo kit file cannot be read: %1", path);
    case engine::DrumGizmoError::notADrumkit:
        return i18n::tr("The file is not a DrumGizmo kit (drumkit.xml): %1", path);
    case engine::DrumGizmoError::instrumentMissing:
        return i18n::tr("An instrument file of the DrumGizmo kit is missing: %1", path);
    case engine::DrumGizmoError::instrumentBroken:
        return i18n::tr("An instrument file of the DrumGizmo kit cannot be read: %1", path);
    case engine::DrumGizmoError::outOfMemory:
        return i18n::tr("The DrumGizmo kit does not fit into RAM: %1", path);
    default:
        return {};
    }
}

void KitPresenter::tick()
{
    engine_.collectGarbage();
    if (kits_.poll())
        ++changeCount_;
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

std::string KitPresenter::drumGizmoKitName() const
{
    // Kits without a name in drumkit.xml are named by their folder.
    if (const auto& read = drumGizmoKit(); read.kit && !read.kit->name.empty())
        return read.kit->name;
    const auto file = io::pathFromUtf8(activeKit().drumkitFile());
    return io::utf8FromPath(file.parent_path().filename());
}

const engine::DrumGizmoReadResult& KitPresenter::drumGizmoKit() const
{
    const auto file = io::pathFromUtf8(activeKit().drumkitFile());
    if (file != readFile_)
    {
        readFile_ = file;
        readKit_ = drumGizmoReader_.read(file, false);
    }
    return readKit_;
}

void KitPresenter::beginEdit(const model::SampleSlot& slot, const char* what)
{
    const auto name = i18n::trJuce(what, slot.name());
    if (undoManager_.getCurrentTransactionName() != name)
        undoManager_.beginNewTransaction(name);
}

} // namespace drumprog::ui
