#pragma once

#include "ui/IMidiImportView.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Answers the file dialog immediately with a preset answer and records what was shown.
class FakeMidiImportView final : public IMidiImportView
{
public:
    struct Message
    {
        std::string title;
        std::string text;
    };

    void chooseMidiFileToOpen(FileChosenCallback onChosen) override
    {
        ++numFileDialogs;
        onChosen(fileToOpen);
    }

    void showMessage(const std::string& title, const std::string& message) override
    {
        messages.push_back({title, message});
    }

    // Preset answer
    std::optional<std::filesystem::path> fileToOpen;

    // Recorded calls
    int numFileDialogs = 0;
    std::vector<Message> messages;
};

} // namespace drumprog::ui
