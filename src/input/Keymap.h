#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::input
{

/// Which physical key (scancode) triggers which MIDI note (Pflichtenheft chapter 4: Keymap). It is a
/// program setting, not part of the project, because it belongs to the keyboard (F-IN-02).
/// A note has at most one key; a key may be given to several notes, which is reported as duplicate.
class Keymap
{
public:
    /// Default mapping from the GUI draft (Pflichtenheft 6.3).
    [[nodiscard]] static Keymap defaults();
    /// Reads the text written by toText(); no saved text yet gives the defaults. Malformed lines and
    /// values outside the MIDI or scancode range are skipped.
    [[nodiscard]] static Keymap fromText(const std::optional<std::string>& text);

    [[nodiscard]] std::optional<int> keyFor(int midiNote) const;
    /// Notes the key triggers, in ascending order.
    [[nodiscard]] std::vector<int> notesFor(int scancode) const;
    /// Keys given to more than one note, each with its notes (F-IN-02).
    [[nodiscard]] std::map<int, std::vector<int>> duplicates() const;

    /// Ignores notes outside 0..127 and invalid scancodes.
    void assign(int midiNote, int scancode);
    void clear(int midiNote);

    /// One "note=scancode" line per assigned note.
    [[nodiscard]] std::string toText() const;

private:
    std::map<int, int> keys_; ///< midiNote -> scancode
};

} // namespace drumprog::input
