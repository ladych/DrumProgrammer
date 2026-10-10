#pragma once

#include "ui/IDocumentView.h"

#include <functional>
#include <string>

namespace drumprog::ui
{

/// Dialogs the MidiImportController needs; asynchronous like IDocumentView.
class IMidiImportView
{
public:
    virtual ~IMidiImportView() = default;

    /// Reports std::nullopt if the user cancels.
    virtual void chooseMidiFileToOpen(FileChosenCallback onChosen) = 0;
    /// Asks whether the project takes tempo and time signature of the file; reports the answer.
    virtual void askToAdoptTempo(const std::string& question, std::function<void(bool)> onAnswer) = 0;
    virtual void showMessage(const std::string& title, const std::string& message) = 0;

protected:
    IMidiImportView() = default;
    IMidiImportView(const IMidiImportView&) = default;
    IMidiImportView(IMidiImportView&&) = default;
    IMidiImportView& operator=(const IMidiImportView&) = default;
    IMidiImportView& operator=(IMidiImportView&&) = default;
};

} // namespace drumprog::ui
