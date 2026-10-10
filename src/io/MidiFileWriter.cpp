#include "io/MidiFileWriter.h"

#include <algorithm>
#include <cmath>

namespace drumprog::io
{
namespace
{

constexpr std::uint8_t kNoteOn = 0x99;  // channel 10
constexpr std::uint8_t kNoteOff = 0x89; // channel 10
constexpr std::uint8_t kMeta = 0xFF;
constexpr std::uint8_t kMetaTrackName = 0x03;
constexpr std::uint8_t kMetaEndOfTrack = 0x2F;
constexpr std::uint8_t kMetaTempo = 0x51;
constexpr std::uint8_t kMetaTimeSignature = 0x58;
constexpr int kMidiClocksPerWholeNote = 96;
constexpr int kThirtySecondsPerQuarter = 8;
constexpr double kMicrosecondsPerMinute = 60'000'000.0;
constexpr int kSevenBits = 7;
constexpr std::uint8_t kLow7Bits = 0x7F;
constexpr std::uint8_t kMoreBytes = 0x80;
constexpr int kBitsPerByte = 8;
constexpr std::uint8_t kByteMask = 0xFF;

struct Event
{
    std::int64_t tick = 0;
    std::string bytes;
};

void appendBigEndian(std::string& out, std::uint32_t value, int numBytes)
{
    for (int shift = (numBytes - 1) * kBitsPerByte; shift >= 0; shift -= kBitsPerByte)
        out.push_back(static_cast<char>((value >> shift) & kByteMask));
}

/// Variable-length quantity: 7 bits per byte, most significant first, high bit set on all but the last.
void appendVariableLength(std::string& out, std::uint64_t value)
{
    std::string reversed(1, static_cast<char>(value & kLow7Bits));
    while ((value >>= kSevenBits) != 0)
        reversed.push_back(static_cast<char>((value & kLow7Bits) | kMoreBytes));
    out.append(reversed.rbegin(), reversed.rend());
}

std::string metaEvent(std::uint8_t type, const std::string& data)
{
    std::string bytes{static_cast<char>(kMeta), static_cast<char>(type)};
    appendVariableLength(bytes, data.size());
    return bytes + data;
}

std::string noteEvent(std::uint8_t status, int note, int velocity)
{
    return {static_cast<char>(status), static_cast<char>(note), static_cast<char>(velocity)};
}

int log2Of(int powerOfTwo)
{
    int power = 0;
    while ((1 << power) < powerOfTwo)
        ++power;
    return power;
}

std::string timeSignature(const MidiPattern& pattern)
{
    return {static_cast<char>(pattern.timeSigNumerator),
            static_cast<char>(log2Of(pattern.timeSigDenominator)),
            static_cast<char>(kMidiClocksPerWholeNote / pattern.timeSigDenominator),
            static_cast<char>(kThirtySecondsPerQuarter)};
}

std::string tempo(double bpm)
{
    std::string bytes;
    appendBigEndian(bytes, static_cast<std::uint32_t>(std::lround(kMicrosecondsPerMinute / bpm)), 3);
    return bytes;
}

/// Note-offs come before note-ons at the same tick, so a note ends before the next one starts.
std::vector<Event> noteEvents(const MidiPattern& pattern)
{
    std::vector<Event> offs;
    std::vector<Event> ons;
    for (const auto& note : pattern.notes)
    {
        if (note.startTick >= pattern.lengthTicks)
            continue;
        const auto end =
            std::min(note.startTick + std::max<std::int64_t>(note.lengthTicks, 1), pattern.lengthTicks);
        ons.push_back({note.startTick, noteEvent(kNoteOn, note.note, note.velocity)});
        offs.push_back({end, noteEvent(kNoteOff, note.note, 0)});
    }
    std::vector<Event> events = std::move(offs);
    events.insert(events.end(), ons.begin(), ons.end());
    std::ranges::stable_sort(events, {}, &Event::tick);
    return events;
}

std::string trackData(const MidiPattern& pattern)
{
    std::vector<Event> events;
    if (!pattern.name.empty())
        events.push_back({0, metaEvent(kMetaTrackName, pattern.name)});
    events.push_back({0, metaEvent(kMetaTimeSignature, timeSignature(pattern))});
    events.push_back({0, metaEvent(kMetaTempo, tempo(pattern.bpm))});
    for (auto& event : noteEvents(pattern))
        events.push_back(std::move(event));
    events.push_back({pattern.lengthTicks, metaEvent(kMetaEndOfTrack, {})});

    std::string data;
    std::int64_t tick = 0;
    for (const auto& event : events)
    {
        appendVariableLength(data, static_cast<std::uint64_t>(event.tick - tick));
        data += event.bytes;
        tick = event.tick;
    }
    return data;
}

} // namespace

std::string writeMidiFile(const MidiPattern& pattern)
{
    const auto track = trackData(pattern);

    std::string file{"MThd"};
    appendBigEndian(file, 6, 4); // header length
    appendBigEndian(file, 0, 2); // format 0
    appendBigEndian(file, 1, 2); // one track
    appendBigEndian(file, static_cast<std::uint32_t>(pattern.ticksPerQuarter), 2);
    file += "MTrk";
    appendBigEndian(file, static_cast<std::uint32_t>(track.size()), 4);
    return file + track;
}

} // namespace drumprog::io
