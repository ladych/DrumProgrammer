#pragma once

#include "ui/IMidiExportView.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <string>

namespace drumprog::app
{

/// Humble object (E-03): save dialog and message box for the MidiExportController, asynchronous.
/// The dialog starts in the folder of the last export.
class JuceMidiExportView final : public ui::IMidiExportView
{
public:
    void chooseMidiFileToSave(const std::string& suggestedFileName, ui::FileChosenCallback onChosen) override;
    void showMessage(const std::string& title, const std::string& message) override;

private:
    juce::File directory_ = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    std::unique_ptr<juce::FileChooser> chooser_;
};

} // namespace drumprog::app
