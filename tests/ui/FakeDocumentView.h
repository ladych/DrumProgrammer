#pragma once

#include "ui/IDocumentView.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Answers every dialog immediately with preset answers and records what was shown.
class FakeDocumentView final : public IDocumentView
{
public:
    struct Message
    {
        std::string title;
        std::string text;
    };

    void setWindowTitle(const std::string& title) override { windowTitle = title; }

    void askToSaveChanges(const std::string& projectName, std::function<void(SaveChangesChoice)> onChoice) override
    {
        askedToSave.push_back(projectName);
        onChoice(saveChangesAnswer);
    }

    void chooseFileToOpen(FileChosenCallback onChosen) override { onChosen(fileToOpen); }

    void chooseFileToSave(const std::string& suggestedFileName, FileChosenCallback onChosen) override
    {
        suggestedSaveNames.push_back(suggestedFileName);
        onChosen(fileToSave);
    }

    void showMessage(const std::string& title, const std::string& message) override
    {
        messages.push_back({title, message});
    }

    // Preset answers
    SaveChangesChoice saveChangesAnswer = SaveChangesChoice::cancel;
    std::optional<std::filesystem::path> fileToOpen;
    std::optional<std::filesystem::path> fileToSave;

    // Recorded calls
    std::string windowTitle;
    std::vector<std::string> askedToSave;
    std::vector<std::string> suggestedSaveNames;
    std::vector<Message> messages;
};

} // namespace drumprog::ui
