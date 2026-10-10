#pragma once

#include "ui/IMidiImportView.h"

#include <filesystem>
#include <functional>
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

    void askToAdoptTempo(const std::string& question, std::function<void(bool)> onAnswer) override
    {
        questions.push_back(question);
        onAnswer(adoptTempo);
    }

    void showMessage(const std::string& title, const std::string& message) override
    {
        messages.push_back({title, message});
    }

    // Preset answer
    std::optional<std::filesystem::path> fileToOpen;
    bool adoptTempo = false;

    // Recorded calls
    int numFileDialogs = 0;
    std::vector<std::string> questions;
    std::vector<Message> messages;
};

} // namespace drumprog::ui
