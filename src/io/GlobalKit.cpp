#include "io/GlobalKit.h"

#include "io/MissingSamples.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

namespace drumprog::io
{

GlobalKit::GlobalKit(SettingsStore& store, const IFileSystem& fileSystem)
    : store_(store), kit_(load(store)), missing_(markMissingSamples(model::Kit{kit_, nullptr}, fileSystem)),
      listener_(kit_, [this] { changed_ = true; })
{
}

bool GlobalKit::saveIfChanged()
{
    if (!changed_)
        return true;
    if (!store_.save(toXml(kit_)))
        return false;
    changed_ = false;
    return true;
}

std::string GlobalKit::toXml(const juce::ValueTree& kit)
{
    auto copy = kit.createCopy();
    for (auto slot : copy)
        slot.removeProperty(model::ids::sampleMissing, nullptr);
    return copy.toXmlString().toStdString();
}

juce::ValueTree GlobalKit::load(const SettingsStore& store)
{
    if (const auto xml = store.load())
    {
        auto kit = juce::ValueTree::fromXml(juce::String::fromUTF8(xml->c_str()));
        if (kit.hasType(model::ids::kit))
            return kit;
    }
    return model::ProjectFactory::createDefaultKit();
}

} // namespace drumprog::io
