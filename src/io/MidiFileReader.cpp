#include "io/MidiFileReader.h"

#include <algorithm>
#include <utility>

namespace drumprog::io
{
namespace
{

constexpr std::size_t kHeaderLength = 6;
constexpr std::uint32_t kSmpteDivision = 0x8000;
constexpr std::uint8_t kStatusBit = 0x80;
constexpr std::uint8_t kMessageMask = 0xF0;
constexpr std::uint8_t kChannelMask = 0x0F;
constexpr std::uint8_t kNoteOff = 0x80;
constexpr std::uint8_t kNoteOn = 0x90;
constexpr std::uint8_t kProgramChange = 0xC0;
constexpr std::uint8_t kChannelPressure = 0xD0;
constexpr std::uint8_t kSystemExclusive = 0xF0;
constexpr std::uint8_t kSystemExclusiveEscape = 0xF7;
constexpr std::uint8_t kMeta = 0xFF;
constexpr std::uint8_t kMetaTrackName = 0x03;
constexpr std::uint8_t kMetaEndOfTrack = 0x2F;
constexpr int kDrumChannel = 9; // channel 10, counted from 0
constexpr int kMaxVariableLengthBytes = 4;
constexpr int kSevenBits = 7;
constexpr std::uint8_t kLow7Bits = 0x7F;
constexpr int kBitsPerByte = 8;

/// Reads a range of the file. Reading past its end or invalid data marks the whole file as failed and
/// moves to the end of the range, so every loop over it stops; the values read then are 0.
class ByteReader
{
public:
    ByteReader(const std::string& data, std::size_t begin, std::size_t end, bool& failed)
        : data_(data), pos_(begin), end_(std::min(end, data.size())), failed_(failed)
    {
    }

    [[nodiscard]] bool atEnd() const noexcept { return pos_ >= end_; }
    [[nodiscard]] std::size_t position() const noexcept { return pos_; }

    void fail() noexcept
    {
        failed_ = true;
        pos_ = end_;
    }

    [[nodiscard]] std::uint8_t peek()
    {
        if (atEnd())
        {
            fail();
            return 0;
        }
        return static_cast<std::uint8_t>(data_[pos_]);
    }

    std::uint8_t next()
    {
        const auto value = peek();
        pos_ = std::min(pos_ + 1, end_);
        return value;
    }

    std::uint32_t bigEndian(int numBytes)
    {
        std::uint32_t value = 0;
        for (int index = 0; index < numBytes; ++index)
            value = (value << kBitsPerByte) | next();
        return value;
    }

    /// Variable-length quantity of at most four bytes.
    std::uint32_t variableLength()
    {
        std::uint32_t value = 0;
        for (int index = 0; index < kMaxVariableLengthBytes; ++index)
        {
            const auto part = next();
            value = (value << kSevenBits) | (part & kLow7Bits);
            if ((part & kStatusBit) == 0)
                return value;
        }
        fail();
        return 0;
    }

    std::string text(std::size_t size)
    {
        if (size > end_ - pos_)
        {
            fail();
            return {};
        }
        auto result = data_.substr(pos_, size);
        pos_ += size;
        return result;
    }

private:
    const std::string& data_;
    std::size_t pos_;
    std::size_t end_;
    bool& failed_;
};

struct ChannelNote
{
    int channel = 0;
    MidiNote note;
};

/// Notes and name collected from all tracks.
struct Collected
{
    std::string name;
    std::int64_t lengthTicks = 0;
    std::vector<ChannelNote> notes;
};

class TrackReader
{
public:
    TrackReader(ByteReader track, Collected& collected) : track_(track), collected_(collected) {}

    void read()
    {
        while (!track_.atEnd() && !ended_)
        {
            tick_ += track_.variableLength();
            readEvent(status());
        }
        for (const auto& open : open_)
            close(open, tick_);
        collected_.lengthTicks = std::max(collected_.lengthTicks, tick_);
    }

private:
    /// The status of the next event; without one the last channel status repeats (running status).
    std::uint8_t status()
    {
        if ((track_.peek() & kStatusBit) != 0)
            return track_.next();
        if (runningStatus_ == 0)
            track_.fail(); // data byte without status
        return runningStatus_;
    }

    void readEvent(std::uint8_t status)
    {
        if (status == kMeta)
            readMeta();
        else if (status == kSystemExclusive || status == kSystemExclusiveEscape)
            track_.text(track_.variableLength());
        else if (status >= kSystemExclusive)
            track_.fail(); // system common or real-time message, not allowed in files
        else
            readChannelMessage(status);
        if (status >= kSystemExclusive)
            runningStatus_ = 0;
    }

    void readMeta()
    {
        const auto type = track_.next();
        const auto data = track_.text(track_.variableLength());
        if (type == kMetaTrackName && collected_.name.empty())
            collected_.name = data;
        ended_ = type == kMetaEndOfTrack;
    }

    void readChannelMessage(std::uint8_t status)
    {
        runningStatus_ = status;
        const auto message = status & kMessageMask;
        const int channel = status & kChannelMask;
        const int key = track_.next();
        if (message == kProgramChange || message == kChannelPressure)
            return;
        const int velocity = track_.next();
        if (message == kNoteOn && velocity > 0)
            open_.push_back(
                {channel, {.note = key, .startTick = tick_, .lengthTicks = 0, .velocity = velocity}});
        else if (message == kNoteOff || message == kNoteOn)
            noteOff(channel, key);
    }

    /// Ends the earliest open note of that key and channel.
    void noteOff(int channel, int key)
    {
        const auto open = std::ranges::find_if(
            open_, [&](const ChannelNote& note) { return note.channel == channel && note.note.note == key; });
        if (open == open_.end())
            return;
        close(*open, tick_);
        open_.erase(open);
    }

    void close(ChannelNote note, std::int64_t endTick)
    {
        note.note.lengthTicks = endTick - note.note.startTick;
        collected_.notes.push_back(note);
    }

    ByteReader track_;
    Collected& collected_;
    std::int64_t tick_ = 0;
    std::uint8_t runningStatus_ = 0;
    bool ended_ = false;
    std::vector<ChannelNote> open_;
};

/// The notes on channel 10 if there are any, otherwise all, sorted by start.
std::vector<MidiNote> drumNotes(const std::vector<ChannelNote>& notes)
{
    const bool hasDrums =
        std::ranges::any_of(notes, [](const auto& note) { return note.channel == kDrumChannel; });
    std::vector<MidiNote> result;
    for (const auto& note : notes)
        if (!hasDrums || note.channel == kDrumChannel)
            result.push_back(note.note);
    std::ranges::stable_sort(result, {}, &MidiNote::startTick);
    return result;
}

/// The division, or 0 if the header does not describe a type 0 or 1 file with ticks per quarter.
int readHeader(ByteReader& file)
{
    if (file.text(4) != "MThd")
        return 0;
    const auto length = file.bigEndian(4);
    if (length < kHeaderLength)
        return 0;
    const auto format = file.bigEndian(2);
    file.bigEndian(2); // number of tracks: every MTrk chunk is read
    const auto division = file.bigEndian(2);
    file.text(length - kHeaderLength);
    if (format > 1 || (division & kSmpteDivision) != 0)
        return 0;
    return static_cast<int>(division);
}

} // namespace

std::optional<MidiFileContent> readMidiFile(const std::string& bytes)
{
    bool failed = false;
    ByteReader file{bytes, 0, bytes.size(), failed};
    const int division = readHeader(file);
    Collected collected;
    while (division != 0 && !file.atEnd())
    {
        const auto type = file.text(4);
        const auto length = file.bigEndian(4);
        const auto begin = file.position();
        const auto end = begin + std::min<std::size_t>(length, bytes.size() - begin);
        if (type == "MTrk")
            TrackReader{ByteReader{bytes, begin, end, failed}, collected}.read();
        file.text(end - begin);
    }
    if (division == 0 || failed)
        return std::nullopt;

    MidiFileContent content;
    content.name = std::move(collected.name);
    content.ticksPerQuarter = division;
    content.lengthTicks = collected.lengthTicks;
    content.notes = drumNotes(collected.notes);
    return content;
}

} // namespace drumprog::io
