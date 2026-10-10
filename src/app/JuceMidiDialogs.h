#pragma once

#include "ui/IMidiExportView.h"
#include "ui/IMidiImportView.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <string>

namespace drumprog::app
{

/// Humble object (E-03): export choice, file dialogs and message boxes for ui::MidiExportController and
/// ui::MidiImportController, asynchronous. The file dialogs start in the folder of the last import or export.
class JuceMidiDialogs final : public ui::IMidiExportView, public ui::IMidiImportView
{
public:
    void
    chooseExportKind(bool canExportPattern, bool canExportSong, ui::ExportKindCallback onChosen) override;
    void chooseMidiFileToSave(const std::string& title,
                              const std::string& suggestedFileName,
                              ui::FileChosenCallback onChosen) override;
    /// Shows the wait cursor while the chosen file is imported.
    void chooseMidiFileToOpen(ui::FileChosenCallback onChosen) override;
    /// Shows the wait cursor while the pattern is added after the answer.
    void askToAdoptTempo(const std::string& question, std::function<void(bool)> onAnswer) override;
    void showMessage(const std::string& title, const std::string& message) override;

private:
    void
    launch(const juce::String& title, const juce::File& start, int flags, ui::FileChosenCallback onChosen);

    juce::File directory_ = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    std::unique_ptr<juce::FileChooser> chooser_;
};

} // namespace drumprog::app
