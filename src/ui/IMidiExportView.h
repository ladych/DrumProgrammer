#pragma once

#include "ui/IDocumentView.h"

#include <string>

namespace drumprog::ui
{

/// Dialogs the MidiExportController needs; asynchronous like IDocumentView.
class IMidiExportView
{
public:
    virtual ~IMidiExportView() = default;

    /// Reports std::nullopt if the user cancels.
    virtual void chooseMidiFileToSave(const std::string& suggestedFileName, FileChosenCallback onChosen) = 0;
    virtual void showMessage(const std::string& title, const std::string& message) = 0;

protected:
    IMidiExportView() = default;
    IMidiExportView(const IMidiExportView&) = default;
    IMidiExportView(IMidiExportView&&) = default;
    IMidiExportView& operator=(const IMidiExportView&) = default;
    IMidiExportView& operator=(IMidiExportView&&) = default;
};

} // namespace drumprog::ui
