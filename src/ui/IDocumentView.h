#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace drumprog::ui
{

enum class SaveChangesChoice : std::uint8_t
{
    save,
    discard,
    cancel
};

using FileChosenCallback = std::function<void(std::optional<std::filesystem::path>)>;

/// Window title and dialogs the DocumentController needs. Dialogs are asynchronous: each reports
/// the user's answer through its callback, so the JUCE implementation needs no modal loops.
class IDocumentView
{
public:
    virtual ~IDocumentView() = default;

    virtual void setWindowTitle(const std::string& title) = 0;
    virtual void askToSaveChanges(const std::string& projectName,
                                  std::function<void(SaveChangesChoice)> onChoice) = 0;
    /// Reports std::nullopt if the user cancels.
    virtual void chooseFileToOpen(FileChosenCallback onChosen) = 0;
    /// Reports std::nullopt if the user cancels.
    virtual void chooseFileToSave(const std::string& suggestedFileName, FileChosenCallback onChosen) = 0;
    virtual void showMessage(const std::string& title, const std::string& message) = 0;

protected:
    IDocumentView() = default;
    IDocumentView(const IDocumentView&) = default;
    IDocumentView(IDocumentView&&) = default;
    IDocumentView& operator=(const IDocumentView&) = default;
    IDocumentView& operator=(IDocumentView&&) = default;
};

} // namespace drumprog::ui
