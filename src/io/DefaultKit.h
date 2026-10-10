#pragma once

#include "io/IFileSystem.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace drumprog::io
{

/// Sample of the default kit for one GM slot.
struct DefaultKitSample
{
    int gmNote;
    std::string_view fileName;
};

/// The program's default kit: the GM default kit (F-SE-04) with the samples of "Black Pearl" (AVL
/// Drumkits by Glen MacArthur, CC BY-SA 3.0), one sample per instrument from its highest velocity layer.
/// The samples are built into the program and copied into a directory of the settings, so the kit refers
/// to them by absolute path like any other kit. GUI thread only.
class DefaultKit
{
public:
    /// Returns the bytes of a built-in file by its file name, or nothing if there is none.
    using ResourceReader = std::function<std::optional<std::string>(std::string_view fileName)>;

    /// The attribution and license text copied next to the samples.
    static constexpr std::string_view kReadmeFile = "LIESMICH.txt";

    DefaultKit(IFileSystem& fileSystem, std::filesystem::path directory, ResourceReader resources);

    /// Slots with a sample; the other GM slots of the default kit stay empty.
    [[nodiscard]] static std::span<const DefaultKitSample> samples();

    /// Copies the built-in files that are not in the directory yet. Returns false if one could not be
    /// copied; the kit then reports its sample as missing.
    bool install();

    /// The GM default kit with the samples of the directory assigned.
    [[nodiscard]] juce::ValueTree createKit() const;

private:
    bool installFile(std::string_view fileName);

    IFileSystem& fileSystem_;
    std::filesystem::path directory_;
    ResourceReader resources_;
};

} // namespace drumprog::io
