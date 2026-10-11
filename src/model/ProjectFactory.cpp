#include "model/ProjectFactory.h"

#include "engine/KitDescription.h"
#include "i18n/Translate.h"
#include "model/ModelIds.h"
#include "model/Project.h"

namespace drumprog::model
{
namespace
{

constexpr const char* kDefaultProjectName = i18n::marked("New project");

} // namespace

ProjectFactory::ProjectFactory(IIdGenerator& idGenerator) : idGenerator_(idGenerator) {}

juce::ValueTree ProjectFactory::createDefault() const
{
    juce::ValueTree tree{ids::project};
    tree.setProperty(ids::name, i18n::trJuce(kDefaultProjectName), nullptr);
    tree.setProperty(ids::bpm, kDefaultBpm, nullptr);
    tree.setProperty(ids::timeSigNumerator, 4, nullptr);
    tree.setProperty(ids::timeSigDenominator, 4, nullptr);
    tree.setProperty(ids::ppq, kTicksPerQuarter, nullptr);
    tree.appendChild(juce::ValueTree{ids::patterns}, nullptr);
    tree.appendChild(juce::ValueTree{ids::song}, nullptr);
    tree.appendChild(juce::ValueTree{ids::backingTrack}, nullptr);
    tree.appendChild(juce::ValueTree{ids::mix}, nullptr);

    Project{tree, nullptr}.addPattern(idGenerator_.next(), i18n::tr("Pattern %1", "1"), kDefaultPatternBars);
    return tree;
}

juce::ValueTree ProjectFactory::createDefaultKit()
{
    juce::ValueTree kit{ids::kit};
    for (const auto& description : engine::makeGmDefaultKit())
    {
        juce::ValueTree slot{ids::slot};
        slot.setProperty(ids::gmNote, description.gmNote, nullptr);
        slot.setProperty(ids::midiNote, description.midiNote, nullptr);
        slot.setProperty(ids::name, juce::String::fromUTF8(description.name.c_str()), nullptr);
        slot.setProperty(ids::chokeGroup, description.chokeGroup, nullptr);
        kit.appendChild(slot, nullptr);
    }
    return kit;
}

} // namespace drumprog::model
