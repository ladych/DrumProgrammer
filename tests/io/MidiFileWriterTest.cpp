#include "io/MidiFileWriter.h"
#include "io/SmfReader.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace drumprog::io
{
namespace
{

using Bytes = std::vector<std::uint8_t>;

constexpr std::int64_t kBar = 3840;

MidiPattern beat()
{
    return {.name = "Beat",
            .bpm = 120.0,
            .timeSigNumerator = 4,
            .timeSigDenominator = 4,
            .ticksPerQuarter = 960,
            .lengthTicks = kBar,
            .notes = {{.note = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100},
                      {.note = 38, .startTick = 1920, .lengthTicks = 240, .velocity = 90}}};
}

std::string bytesOf(const Bytes& bytes)
{
    return {bytes.begin(), bytes.end()};
}

/// Note events only, as (tick, status, note, velocity).
std::vector<SmfEvent> noteEvents(const MidiPattern& pattern)
{
    std::vector<SmfEvent> notes;
    for (const auto& event : readSmf(writeMidiFile(pattern)).events)
        if (event.bytes.front() != 0xFF)
            notes.push_back(event);
    return notes;
}

const SmfEvent* findMeta(const SmfContent& content, std::uint8_t type)
{
    for (const auto& event : content.events)
        if (event.bytes.at(0) == 0xFF && event.bytes.at(1) == type)
            return &event;
    return nullptr;
}

TEST(MidiFileWriterTest, FMI01_WritesTheWholeFileAsSmfType0)
{
    // clang-format off
    const Bytes expected{
        // Header: type 0, one track, 960 ticks per quarter
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0x03, 0xC0,
        // Track with 47 bytes
        'M', 'T', 'r', 'k', 0, 0, 0, 47,
        0x00, 0xFF, 0x03, 4, 'B', 'e', 'a', 't', // track name
        0x00, 0xFF, 0x58, 4, 4, 2, 24, 8,        // 4/4, one click per quarter
        0x00, 0xFF, 0x51, 3, 0x07, 0xA1, 0x20,   // 500000 us per quarter = 120 BPM
        0x00, 0x99, 36, 100,                     // kick on, channel 10
        0x81, 0x70, 0x89, 36, 0,                 // +240: kick off
        0x8D, 0x10, 0x99, 38, 90,                // +1680: snare on
        0x81, 0x70, 0x89, 38, 0,                 // +240: snare off
        0x8D, 0x10, 0xFF, 0x2F, 0};              // +1680: end of track at the end of the bar
    // clang-format on

    EXPECT_EQ(writeMidiFile(beat()), bytesOf(expected));
}

TEST(MidiFileWriterTest, FMI01_HeaderTakesTheTicksPerQuarterOfTheProject)
{
    auto pattern = beat();
    pattern.ticksPerQuarter = 480;

    const auto content = readSmf(writeMidiFile(pattern));

    EXPECT_EQ(content.format, 0);
    EXPECT_EQ(content.numTracks, 1);
    EXPECT_EQ(content.division, 480);
    EXPECT_TRUE(content.trackLengthMatches);
}

TEST(MidiFileWriterTest, FMI01_WritesTheDenominatorAsPowerOfTwoWithOneClickPerBeat)
{
    auto pattern = beat();
    pattern.timeSigNumerator = 6;
    pattern.timeSigDenominator = 8;
    const auto sixEight = readSmf(writeMidiFile(pattern));
    pattern.timeSigNumerator = 7;
    pattern.timeSigDenominator = 16;
    const auto sevenSixteen = readSmf(writeMidiFile(pattern));

    ASSERT_NE(findMeta(sixEight, 0x58), nullptr);
    ASSERT_NE(findMeta(sevenSixteen, 0x58), nullptr);
    EXPECT_EQ(findMeta(sixEight, 0x58)->bytes, (Bytes{0xFF, 0x58, 4, 6, 3, 12, 8}));
    EXPECT_EQ(findMeta(sevenSixteen, 0x58)->bytes, (Bytes{0xFF, 0x58, 4, 7, 4, 6, 8}));
}

TEST(MidiFileWriterTest, FMI01_WritesTheTempoRoundedToWholeMicroseconds)
{
    auto pattern = beat();
    pattern.bpm = 133.33; // 450011.25 us per quarter

    const auto content = readSmf(writeMidiFile(pattern));

    ASSERT_NE(findMeta(content, 0x51), nullptr);
    EXPECT_EQ(findMeta(content, 0x51)->bytes, (Bytes{0xFF, 0x51, 3, 0x06, 0xDD, 0xDB}));
}

TEST(MidiFileWriterTest, FMI01_EndsTheTrackAtThePatternEndAlsoWithoutNotes)
{
    auto pattern = beat();
    pattern.notes.clear();
    pattern.lengthTicks = 2 * kBar;

    const auto content = readSmf(writeMidiFile(pattern));

    ASSERT_FALSE(content.events.empty());
    EXPECT_EQ(content.events.back().tick, 2 * kBar);
    EXPECT_EQ(content.events.back().bytes, (Bytes{0xFF, 0x2F, 0}));
    EXPECT_TRUE(noteEvents(pattern).empty());
}

TEST(MidiFileWriterTest, FMI01_SortsNotesByTimeAndEndsANoteBeforeTheNextOneStarts)
{
    auto pattern = beat();
    pattern.notes = {{.note = 42, .startTick = 240, .lengthTicks = 240, .velocity = 70},
                     {.note = 42, .startTick = 0, .lengthTicks = 240, .velocity = 80}};

    const auto events = noteEvents(pattern);

    ASSERT_EQ(events.size(), 4U);
    EXPECT_EQ(events[0].tick, 0);
    EXPECT_EQ(events[0].bytes, (Bytes{0x99, 42, 80}));
    EXPECT_EQ(events[1].tick, 240);
    EXPECT_EQ(events[1].bytes, (Bytes{0x89, 42, 0}));
    EXPECT_EQ(events[2].tick, 240);
    EXPECT_EQ(events[2].bytes, (Bytes{0x99, 42, 70}));
    EXPECT_EQ(events[3].tick, 480);
    EXPECT_EQ(events[3].bytes, (Bytes{0x89, 42, 0}));
}

TEST(MidiFileWriterTest, FMI01_KeepsTheOrderOfNotesStartingTogether)
{
    auto pattern = beat();
    pattern.notes = {{.note = 49, .startTick = 0, .lengthTicks = 240, .velocity = 110},
                     {.note = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100}};

    const auto events = noteEvents(pattern);

    ASSERT_EQ(events.size(), 4U);
    EXPECT_EQ(events[0].bytes, (Bytes{0x99, 49, 110}));
    EXPECT_EQ(events[1].bytes, (Bytes{0x99, 36, 100}));
}

TEST(MidiFileWriterTest, FMI01_CutsNotesAtThePatternEndAndLeavesOutNotesBehindIt)
{
    auto pattern = beat();
    pattern.notes = {{.note = 36, .startTick = kBar - 120, .lengthTicks = 240, .velocity = 100},
                     {.note = 38, .startTick = kBar, .lengthTicks = 240, .velocity = 100}};

    const auto events = noteEvents(pattern);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].tick, kBar - 120);
    EXPECT_EQ(events[1].tick, kBar);
    EXPECT_EQ(events[1].bytes, (Bytes{0x89, 36, 0}));
}

TEST(MidiFileWriterTest, FMI01_GivesEveryNoteAtLeastOneTick)
{
    auto pattern = beat();
    pattern.notes = {{.note = 36, .startTick = 480, .lengthTicks = 0, .velocity = 100}};

    const auto events = noteEvents(pattern);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].bytes, (Bytes{0x99, 36, 100}));
    EXPECT_EQ(events[1].tick, 481);
    EXPECT_EQ(events[1].bytes, (Bytes{0x89, 36, 0}));
}

TEST(MidiFileWriterTest, FMI01_WritesLongDeltaTimesWithAllBytesOfTheVariableLengthQuantity)
{
    auto pattern = beat();
    pattern.name.clear();
    pattern.notes.clear();
    pattern.lengthTicks = 0x200000;

    const auto file = writeMidiFile(pattern);

    EXPECT_EQ(file.substr(file.size() - 7), bytesOf({0x81, 0x80, 0x80, 0x00, 0xFF, 0x2F, 0}));
    EXPECT_EQ(readSmf(file).events.back().tick, 0x200000);
}

TEST(MidiFileWriterTest, FMI01_WritesLongUtf8NamesWithAVariableLengthSize)
{
    auto pattern = beat();
    pattern.name = std::string(198, 'x') + "\xc3\xa4"; // 200 bytes ending in "ä"

    const auto file = writeMidiFile(pattern);

    EXPECT_EQ(file.substr(22, 5), bytesOf({0x00, 0xFF, 0x03, 0x81, 0x48}));
    EXPECT_EQ(file.substr(27, 200), pattern.name);
}

TEST(MidiFileWriterTest, FMI01_LeavesOutTheTrackNameOfAnUnnamedPattern)
{
    auto pattern = beat();
    pattern.name.clear();

    const auto content = readSmf(writeMidiFile(pattern));

    EXPECT_EQ(findMeta(content, 0x03), nullptr);
    EXPECT_NE(findMeta(content, 0x58), nullptr);
}

} // namespace
} // namespace drumprog::io
