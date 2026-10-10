#include "io/DefaultKit.h"

#include "io/Utf8Path.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <algorithm>
#include <array>
#include <utility>

namespace drumprog::io
{
namespace
{

// Each GM instrument gets the "-5" file, the highest of the kit's five velocity layers. Black Pearl has one
// rack tom and one floor tom, so the six GM toms share them; its edge hits take the second note of each
// tom. GM 35 is a bass drum (the kit has a stick click there) and the china sits on 52. The kit has no
// vibraslap (58); its stick click, hi-hat swish, chokes, Paiste crash and maracas are not used.
constexpr std::array<DefaultKitSample, 24> kSamples{{
    {35, "36-Pearl-22-Kick-5.wav"},
    {36, "36-Pearl-22-Kick-5.wav"},
    {37, "37-Pearl-14-SideStick-5.wav"},
    {38, "38-Pearl-14-Snare-5.wav"},
    {39, "39-HandClap-5.wav"},
    {40, "40-Pearl-14-SnareEdge-5.wav"},
    {41, "41-Pearl-16-FloorTom-5.wav"},
    {42, "42-Sabian-13-HatClosed-5.wav"},
    {43, "43-Pearl-16-FloorTomEdge-5.wav"},
    {44, "44-Sabian-13-HatPedal-5.wav"},
    {45, "45-Pearl-12-Tom-5.wav"},
    {46, "46-Sabian-13-HatSemi-5.wav"},
    {47, "47-Pearl-12-TomEdge-5.wav"},
    {48, "45-Pearl-12-Tom-5.wav"},
    {49, "49-SabianAA-16-Crash-5.wav"},
    {50, "47-Pearl-12-TomEdge-5.wav"},
    {51, "51-SabianAAX-20-Ride-5.wav"},
    {52, "62-Wuhan-18-ChinaCrash-5.wav"},
    {53, "53-SabianAAX-20-RideBell-5.wav"},
    {54, "54-Tambourine-5.wav"},
    {55, "55-Zildjian-10-Splash-5.wav"},
    {56, "56-Cowbell-5.wav"},
    {57, "57-SabianAAX-17-Crash-5.wav"},
    {59, "59-SabianAAX-20-RideShank-5.wav"},
}};

bool usedByEarlierSlot(const DefaultKitSample* sample)
{
    const auto* const begin = kSamples.data();
    return std::any_of(
        begin, sample, [sample](const auto& other) { return other.fileName == sample->fileName; });
}

const DefaultKitSample* findSample(int gmNote)
{
    for (const auto& sample : kSamples)
        if (sample.gmNote == gmNote)
            return &sample;
    return nullptr;
}

} // namespace

DefaultKit::DefaultKit(IFileSystem& fileSystem, std::filesystem::path directory, ResourceReader resources)
    : fileSystem_(fileSystem), directory_(std::move(directory)), resources_(std::move(resources))
{
}

std::span<const DefaultKitSample> DefaultKit::samples()
{
    return kSamples;
}

bool DefaultKit::install()
{
    if (!fileSystem_.createDirectories(directory_))
        return false;
    bool installed = installFile(kReadmeFile);
    for (const auto& sample : kSamples)
        if (!usedByEarlierSlot(&sample))
            installed = installFile(sample.fileName) && installed;
    return installed;
}

juce::ValueTree DefaultKit::createKit() const
{
    auto tree = model::ProjectFactory::createDefaultKit();
    const model::Kit kit{tree, nullptr};
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        auto slot = kit.slot(index);
        if (const auto* sample = findSample(slot.gmNote()))
            slot.setFilePath(utf8FromPath(directory_ / pathFromUtf8(std::string{sample->fileName})));
    }
    return tree;
}

bool DefaultKit::installFile(std::string_view fileName)
{
    const auto file = directory_ / pathFromUtf8(std::string{fileName});
    if (fileSystem_.exists(file))
        return true;
    const auto bytes = resources_(fileName);
    if (!bytes)
        return false;
    // Written under another name first, so an interrupted copy is not taken for a complete file at the next
    // start. writeText writes the bytes unchanged.
    auto partial = file;
    partial += ".part";
    return fileSystem_.writeText(partial, *bytes) && fileSystem_.rename(partial, file);
}

} // namespace drumprog::io
