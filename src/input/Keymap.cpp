#include "input/Keymap.h"

#include "input/Scancode.h"

#include <array>
#include <sstream>
#include <utility>

namespace drumprog::input
{
namespace
{

constexpr int kMaxMidiNote = 127;
constexpr const char* kHeader = "# Drum Programmer Tastatur-Mapping: MIDI-Note=Scancode\n";

constexpr std::array<std::pair<int, int>, 17> kDefaultKeys{{
    {36, scancode::kA},      // Bass Drum 1
    {37, scancode::kX},      // Side Stick
    {38, scancode::kS},      // Acoustic Snare
    {39, scancode::kD},      // Hand Clap
    {41, scancode::kF},      // Low Floor Tom
    {42, scancode::kW},      // Closed Hi-Hat
    {44, scancode::kQ},      // Pedal Hi-Hat
    {45, scancode::kR},      // Low Tom
    {46, scancode::kE},      // Open Hi-Hat
    {48, scancode::kT},      // Hi-Mid Tom
    {49, scancode::kDigit0}, // Crash Cymbal 1
    {50, scancode::kZ},      // High Tom: the key left of X, "Y" on a German keyboard (K5)
    {51, scancode::kI},      // Ride Cymbal 1
    {53, scancode::kU},      // Ride Bell
    {54, scancode::kDigit7}, // Tambourine
    {56, scancode::kDigit8}, // Cowbell
    {57, scancode::kDigit9}, // Crash Cymbal 2
}};

bool isValidNote(int midiNote) noexcept
{
    return midiNote >= 0 && midiNote <= kMaxMidiNote;
}

} // namespace

Keymap Keymap::defaults()
{
    Keymap keymap;
    for (const auto& [note, key] : kDefaultKeys)
        keymap.assign(note, key);
    return keymap;
}

Keymap Keymap::fromText(const std::optional<std::string>& text)
{
    if (!text)
        return defaults();
    Keymap keymap;
    std::istringstream lines{*text};
    std::string line;
    while (std::getline(lines, line))
    {
        std::istringstream fields{line};
        int note = -1;
        char separator = 0;
        int key = 0;
        if (fields >> note >> separator >> key && separator == '=')
            keymap.assign(note, key);
    }
    return keymap;
}

std::optional<int> Keymap::keyFor(int midiNote) const
{
    if (const auto found = keys_.find(midiNote); found != keys_.end())
        return found->second;
    return std::nullopt;
}

std::vector<int> Keymap::notesFor(int scancode) const
{
    std::vector<int> notes;
    for (const auto& [note, key] : keys_)
        if (key == scancode)
            notes.push_back(note);
    return notes;
}

std::map<int, std::vector<int>> Keymap::duplicates() const
{
    std::map<int, std::vector<int>> byKey;
    for (const auto& [note, key] : keys_)
        byKey[key].push_back(note);
    std::erase_if(byKey, [](const auto& entry) { return entry.second.size() < 2; });
    return byKey;
}

void Keymap::assign(int midiNote, int scancode)
{
    if (isValidNote(midiNote) && isValidScancode(scancode))
        keys_[midiNote] = scancode;
}

void Keymap::clear(int midiNote)
{
    keys_.erase(midiNote);
}

std::string Keymap::toText() const
{
    std::string text = kHeader;
    for (const auto& [note, key] : keys_)
        text += std::to_string(note) + "=" + std::to_string(key) + "\n";
    return text;
}

} // namespace drumprog::input
