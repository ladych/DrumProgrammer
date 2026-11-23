#include "io/DrumGizmoReader.h"

#include "io/Utf8Path.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <utility>

namespace drumprog::io
{
using engine::DrumGizmoError;
using engine::DrumGizmoReadResult;

namespace
{

std::unique_ptr<juce::XmlElement> parseXml(const std::optional<std::string>& text)
{
    if (!text)
        return nullptr;
    return juce::XmlDocument::parse(juce::String::fromUTF8(text->c_str()));
}

std::string attribute(const juce::XmlElement& element, const char* name)
{
    return element.getStringAttribute(name).toStdString();
}

std::filesystem::path pathIn(const std::filesystem::path& directory, const std::string& relative)
{
    return (directory / pathFromUtf8(relative)).lexically_normal();
}

std::string lowerCase(std::string text)
{
    std::ranges::transform(
        text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::vector<std::string> readChannels(const juce::XmlElement& drumkit)
{
    std::vector<std::string> channels;
    if (const auto* list = drumkit.getChildByName("channels"))
        for (const auto* channel : list->getChildWithTagNameIterator("channel"))
            channels.push_back(attribute(*channel, "name"));
    return channels;
}

std::vector<DrumGizmoInstrumentEntry> readInstrumentEntries(const juce::XmlElement& drumkit,
                                                            const std::filesystem::path& directory)
{
    std::vector<DrumGizmoInstrumentEntry> entries;
    if (const auto* list = drumkit.getChildByName("instruments"))
        for (const auto* instrument : list->getChildWithTagNameIterator("instrument"))
        {
            DrumGizmoInstrumentEntry entry;
            entry.name = attribute(*instrument, "name");
            entry.group = attribute(*instrument, "group");
            entry.file = pathIn(directory, attribute(*instrument, "file"));
            for (const auto* map : instrument->getChildWithTagNameIterator("channelmap"))
                entry.channelMap[attribute(*map, "in")] = attribute(*map, "out");
            entries.push_back(std::move(entry));
        }
    return entries;
}

/// Format 1.0 keeps the power in velocity ranges that refer to the samples by name.
std::map<std::string, double> velocityPowers(const juce::XmlElement& instrument)
{
    std::map<std::string, double> powers;
    if (const auto* velocities = instrument.getChildByName("velocities"))
        for (const auto* velocity : velocities->getChildWithTagNameIterator("velocity"))
        {
            const double middle =
                (velocity->getDoubleAttribute("lower") + velocity->getDoubleAttribute("upper")) / 2.0;
            for (const auto* reference : velocity->getChildWithTagNameIterator("sampleref"))
                powers.try_emplace(attribute(*reference, "name"), middle);
        }
    return powers;
}

double
powerOf(const juce::XmlElement& sample, const std::string& name, const std::map<std::string, double>& powers)
{
    if (sample.hasAttribute("power"))
        return sample.getDoubleAttribute("power");
    const auto fromVelocity = powers.find(name);
    return fromVelocity != powers.end() ? fromVelocity->second : 0.0;
}

std::string kitChannel(const DrumGizmoInstrumentEntry& entry, const std::string& instrumentChannel)
{
    const auto mapped = entry.channelMap.find(instrumentChannel);
    return mapped != entry.channelMap.end() ? mapped->second : instrumentChannel;
}

engine::DrumGizmoSample readSample(const juce::XmlElement& sample,
                                   const DrumGizmoInstrumentEntry& entry,
                                   const std::map<std::string, double>& powers)
{
    engine::DrumGizmoSample result{.name = attribute(sample, "name"), .power = 0.0, .audioFiles = {}};
    result.power = powerOf(sample, result.name, powers);
    const auto directory = entry.file.parent_path();
    for (const auto* audioFile : sample.getChildWithTagNameIterator("audiofile"))
        result.audioFiles.push_back(
            {.channel = kitChannel(entry, attribute(*audioFile, "channel")),
             .file = pathIn(directory, attribute(*audioFile, "file")),
             .fileChannel = std::max(1, audioFile->getIntAttribute("filechannel", 1))});
    return result;
}

std::vector<engine::DrumGizmoSample> readSamples(const juce::XmlElement& instrument,
                                                 const DrumGizmoInstrumentEntry& entry)
{
    std::vector<engine::DrumGizmoSample> samples;
    const auto powers = velocityPowers(instrument);
    if (const auto* list = instrument.getChildByName("samples"))
        for (const auto* sample : list->getChildWithTagNameIterator("sample"))
            samples.push_back(readSample(*sample, entry, powers));
    return samples;
}

bool isMidiMapFile(const std::filesystem::path& file)
{
    const auto name = lowerCase(utf8FromPath(file.filename()));
    return name.find("midimap") != std::string::npos && name.ends_with(".xml");
}

} // namespace

DrumGizmoReader::DrumGizmoReader(const IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

DrumGizmoReadResult DrumGizmoReader::read(const std::filesystem::path& drumkitFile, bool withSamples) const
{
    const auto text = fileSystem_.readText(drumkitFile);
    if (!text)
        return {.kit = std::nullopt, .error = DrumGizmoError::fileMissing, .errorFile = drumkitFile};
    const auto drumkit = parseXml(text);
    if (drumkit == nullptr || !drumkit->hasTagName("drumkit"))
        return {.kit = std::nullopt, .error = DrumGizmoError::notADrumkit, .errorFile = drumkitFile};

    const auto directory = drumkitFile.parent_path();
    engine::DrumGizmoKit kit;
    kit.name = attribute(*drumkit, "name");
    kit.channels = readChannels(*drumkit);
    kit.midiMap = readMidiMap(directory);
    auto entries = readInstrumentEntries(*drumkit, directory);
    return readInstruments(std::move(kit), entries, withSamples);
}

DrumGizmoReadResult DrumGizmoReader::readInstruments(engine::DrumGizmoKit kit,
                                                     const std::vector<DrumGizmoInstrumentEntry>& entries,
                                                     bool withSamples) const
{
    for (const auto& entry : entries)
    {
        const auto instrument = parseXml(fileSystem_.readText(entry.file));
        if (instrument == nullptr || !instrument->hasTagName("instrument"))
        {
            const bool broken = instrument != nullptr || fileSystem_.exists(entry.file);
            return {.kit = std::nullopt,
                    .error = broken ? DrumGizmoError::instrumentBroken : DrumGizmoError::instrumentMissing,
                    .errorFile = entry.file};
        }
        kit.instruments.push_back({.name = entry.name,
                                   .group = entry.group,
                                   .samples = withSamples ? readSamples(*instrument, entry)
                                                          : std::vector<engine::DrumGizmoSample>{}});
    }
    return {.kit = std::move(kit), .error = DrumGizmoError::none, .errorFile = {}};
}

std::map<int, std::string> DrumGizmoReader::readMidiMap(const std::filesystem::path& directory) const
{
    std::map<int, std::string> midiMap;
    for (const auto& file : fileSystem_.listFiles(directory))
    {
        if (!isMidiMapFile(file))
            continue;
        const auto xml = parseXml(fileSystem_.readText(file));
        if (xml == nullptr || !xml->hasTagName("midimap"))
            continue;
        for (const auto* map : xml->getChildWithTagNameIterator("map"))
            midiMap.try_emplace(map->getIntAttribute("note", -1), attribute(*map, "instr"));
        return midiMap;
    }
    return midiMap;
}

} // namespace drumprog::io
