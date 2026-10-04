#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drumprog::io
{

/// Minimal reader for the files io::writeMidiFile produces, so tests can check events instead of
/// raw bytes. It understands one track with meta events and note on/off without running status.
struct SmfEvent
{
    std::int64_t tick = 0;
    std::vector<std::uint8_t> bytes; ///< status and data, for meta events FF, type and data
};

struct SmfContent
{
    int format = -1;
    int numTracks = 0;
    int division = 0;
    std::vector<SmfEvent> events;
    bool trackLengthMatches = false;
};

inline SmfContent readSmf(const std::string& file)
{
    const auto byte = [&file](std::size_t index) { return static_cast<std::uint8_t>(file.at(index)); };
    const auto read16 = [&byte](std::size_t index) { return (byte(index) << 8) | byte(index + 1); };
    const auto read32 = [&read16](std::size_t index) { return (read16(index) << 16) | read16(index + 2); };

    SmfContent content;
    if (file.substr(0, 4) != "MThd" || read32(4) != 6 || file.substr(14, 4) != "MTrk")
        return content;
    content.format = read16(8);
    content.numTracks = read16(10);
    content.division = read16(12);
    const auto trackLength = static_cast<std::size_t>(read32(18));
    content.trackLengthMatches = file.size() == 22 + trackLength;

    std::size_t pos = 22;
    std::int64_t tick = 0;
    while (pos < file.size())
    {
        std::int64_t delta = 0;
        std::uint8_t part = 0;
        do
        {
            part = byte(pos++);
            delta = (delta << 7) | (part & 0x7F);
        } while ((part & 0x80) != 0);
        tick += delta;

        SmfEvent event{tick, {}};
        const std::uint8_t status = byte(pos);
        std::size_t size = 3;
        if (status == 0xFF)
            size = 3 + byte(pos + 2);
        for (std::size_t index = 0; index < size; ++index)
            event.bytes.push_back(byte(pos + index));
        pos += size;
        content.events.push_back(event);
    }
    return content;
}

} // namespace drumprog::io
