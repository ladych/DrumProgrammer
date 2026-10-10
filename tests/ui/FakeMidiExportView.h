#pragma once

#include "ui/IMidiExportView.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Answers the dialogs immediately with preset answers and records what was shown.
class FakeMidiExportView final : public IMidiExportView
{
public:
    struct Message
    {
        std::string title;
        std::string text;
    };

    struct KindChoice
    {
        bool canExportPattern = false;
        bool canExportSong = false;
    };

    void chooseExportKind(bool canExportPattern, bool canExportSong, ExportKindCallback onChosen) override
    {
        kindChoices.push_back({canExportPattern, canExportSong});
        onChosen(kindToExport);
    }

    void chooseMidiFileToSave(const std::string& title,
                              const std::string& suggestedFileName,
                              FileChosenCallback onChosen) override
    {
        dialogTitles.push_back(title);
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

    // Preset answers
    std::optional<MidiExportKind> kindToExport;
    std::optional<std::filesystem::path> fileToSave;
    bool answerLater = false;
    FileChosenCallback pendingAnswer;

    // Recorded calls
    std::vector<KindChoice> kindChoices;
    std::vector<std::string> dialogTitles;
    std::vector<std::string> suggestedNames;
    std::vector<Message> messages;
};

} // namespace drumprog::ui
