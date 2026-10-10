#pragma once

#include "ui/IMidiExportView.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Answers the file dialog immediately with a preset answer and records what was shown.
class FakeMidiExportView final : public IMidiExportView
{
public:
    struct Message
    {
        std::string title;
        std::string text;
    };

    void chooseMidiFileToSave(const std::string& suggestedFileName, FileChosenCallback onChosen) override
    {
        suggestedNames.push_back(suggestedFileName);
        if (answerLater)
            pendingAnswer = std::move(onChosen);
        else
            onChosen(fileToSave);
    }

    void showMessage(const std::string& title, const std::string& message) override
    {
        messages.push_back({title, message});
    }

    // Preset answer
    std::optional<std::filesystem::path> fileToSave;
    bool answerLater = false;
    FileChosenCallback pendingAnswer;

    // Recorded calls
    std::vector<std::string> suggestedNames;
    std::vector<Message> messages;
};

} // namespace drumprog::ui
