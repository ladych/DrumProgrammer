#pragma once

#include "ui/IDocumentView.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace drumprog::ui
{

/// What the export dialog writes (F-MI-05).
enum class MidiExportKind : std::uint8_t
{
    pattern,
    song
};

/// Reports std::nullopt if the user cancels.
using ExportKindCallback = std::function<void(std::optional<MidiExportKind>)>;

/// Dialogs the MidiExportController needs; asynchronous like IDocumentView.
class IMidiExportView
{
public:
    virtual ~IMidiExportView() = default;

    /// Asks whether to export the active pattern or the song; offers only what is possible.
    virtual void chooseExportKind(bool canExportPattern, bool canExportSong, ExportKindCallback onChosen) = 0;
    /// Reports std::nullopt if the user cancels.
    virtual void chooseMidiFileToSave(const std::string& title,
                                      const std::string& suggestedFileName,
                                      FileChosenCallback onChosen) = 0;
    virtual void showMessage(const std::string& title, const std::string& message) = 0;

protected:
    IMidiExportView() = default;
    IMidiExportView(const IMidiExportView&) = default;
    IMidiExportView(IMidiExportView&&) = default;
    IMidiExportView& operator=(const IMidiExportView&) = default;
    IMidiExportView& operator=(IMidiExportView&&) = default;
};

} // namespace drumprog::ui
