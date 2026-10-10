#include "io/MidiFileReader.h"

#include "io/MidiFileWriter.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace drumprog::io
{
namespace
{

std::string bytes(std::initializer_list<int> values)
{
    std::string result;
    for (const int value : values)
        result.push_back(static_cast<char>(value));
    return result;
}

std::string bigEndian(std::uint32_t value, int numBytes)
{
    std::string result;
    for (int shift = (numBytes - 1) * 8; shift >= 0; shift -= 8)
        result.push_back(static_cast<char>((value >> shift) & 0xFF));
    return result;
}

std::string header(int format, int numTracks, int division)
{
    return "MThd" + bigEndian(6, 4) + bigEndian(static_cast<std::uint32_t>(format), 2) +
           bigEndian(static_cast<std::uint32_t>(numTracks), 2) +
           bigEndian(static_cast<std::uint32_t>(division), 2);
}

std::string track(const std::string& events)
{
    return "MTrk" + bigEndian(static_cast<std::uint32_t>(events.size()), 4) + events;
}

const std::string kEndOfTrack = bytes({0x00, 0xFF, 0x2F, 0x00});

/// "note@start+length/velocity" for every note.
std::vector<std::string> notesOf(const MidiFileContent& content)
{
    std::vector<std::string> result;
    for (const auto& note : content.notes)
        result.push_back(std::to_string(note.note) + "@" + std::to_string(note.startTick) + "+" +
                         std::to_string(note.lengthTicks) + "/" + std::to_string(note.velocity));
    return result;
}

TEST(MidiFileReaderTest, FMI03_ReadsTheFilesTheExportWrites)
{
    MidiPattern pattern;
    pattern.name = "Groove";
    pattern.ticksPerQuarter = 960;
    pattern.lengthTicks = 3840;
    pattern.notes = {{.note = 38, .startTick = 960, .lengthTicks = 240, .velocity = 90},
                     {.note = 36, .startTick = 0, .lengthTicks = 200, .velocity = 127}};

    const auto content = readMidiFile(writeMidiFile(pattern));

    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content->name, "Groove");
    EXPECT_EQ(content->ticksPerQuarter, 960);
    EXPECT_EQ(content->lengthTicks, 3840);
    EXPECT_EQ(notesOf(*content), (std::vector<std::string>{"36@0+200/127", "38@960+240/90"}));
}

TEST(MidiFileReaderTest, FMI03_ReadsTheFirstTempo)
{
    const auto file = header(1, 2, 96) +
                      track(bytes({0x00,
                                   0xFF,
                                   0x51,
                                   0x03,
                                   0x07,
                                   0xA1,
                                   0x20, // 500000 us = 120 BPM
                                   0x00,
                                   0xFF,
                                   0x51,
                                   0x03,
                                   0x0F,
                                   0x42,
                                   0x40}) + // 1000000 us = 60 BPM
                            kEndOfTrack) +
                      track(bytes({0x00, 0xFF, 0x51, 0x03, 0x09, 0x27, 0xC0}) + kEndOfTrack);

    EXPECT_DOUBLE_EQ(readMidiFile(file)->bpm, 120.0);
}

TEST(MidiFileReaderTest, FMI03_IgnoresInvalidTempoEvents)
{
    const auto file = header(0, 1, 96) + track(bytes({0x00, 0xFF, 0x51, 0x02, 0x07, 0xA1,       // too short
                                                      0x00, 0xFF, 0x51, 0x03, 0x00, 0x00, 0x00, // zero
                                                      0x00, 0xFF, 0x51, 0x03, 0x0F, 0x42, 0x40}) +
                                               kEndOfTrack);

    EXPECT_DOUBLE_EQ(readMidiFile(file)->bpm, 60.0);
}

TEST(MidiFileReaderTest, FMI03_AFileWithoutTempoHasNone)
{
    EXPECT_DOUBLE_EQ(readMidiFile(header(0, 1, 96) + track(kEndOfTrack))->bpm, 0.0);
}

TEST(MidiFileReaderTest, FMI03_MergesTheTracksOfAType1File)
{
    const auto file =
        header(1, 3, 480) +
        track(bytes({0x00, 0xFF, 0x03, 0x04, 'S', 'o', 'n', 'g', 0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20}) +
              kEndOfTrack) +
        track(bytes({0x00, 0x99, 42,  80,  0x60, 0x89, 42,   0,    0x00, 0xFF, 0x03,
                     0x05, 'D',  'r', 'u', 'm',  's',  0x00, 0xFF, 0x2F, 0x00})) +
        track(bytes({0x30, 0x99, 36, 100, 0x81, 0x00, 0x89, 36, 0, 0x83, 0x60, 0xFF, 0x2F, 0x00}));

    const auto content = readMidiFile(file);

    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content->name, "Song");
    EXPECT_EQ(content->ticksPerQuarter, 480);
    EXPECT_EQ(content->lengthTicks, 0x30 + 0x80 + 0x1E0);
    EXPECT_EQ(notesOf(*content), (std::vector<std::string>{"42@0+96/80", "36@48+128/100"}));
}

TEST(MidiFileReaderTest, FMI03_TakesTheNameOfTheFirstTrackThatHasOne)
{
    const auto file = header(1, 2, 96) + track(kEndOfTrack) +
                      track(bytes({0x00, 0xFF, 0x03, 0x00, 0x00, 0xFF, 0x03, 0x02, 'B', 'D'}) + kEndOfTrack);

    EXPECT_EQ(readMidiFile(file)->name, "BD");
}

TEST(MidiFileReaderTest, FMI03_TakesOnlyChannel10IfTheFileHasDrumNotes)
{
    const auto file =
        header(0, 1, 96) +
        track(bytes({0x00, 0x90, 60, 100, 0x00, 0x99, 36, 100, 0x10, 0x80, 60, 0, 0x00, 0x89, 36, 0}) +
              kEndOfTrack);

    EXPECT_EQ(notesOf(*readMidiFile(file)), std::vector<std::string>{"36@0+16/100"});
}

TEST(MidiFileReaderTest, FMI03_TakesEveryChannelWithoutDrumNotes)
{
    const auto file =
        header(0, 1, 96) +
        track(bytes({0x00, 0x90, 36, 100, 0x00, 0x93, 38, 70, 0x10, 0x80, 36, 0, 0x00, 0x83, 38, 0}) +
              kEndOfTrack);

    EXPECT_EQ(notesOf(*readMidiFile(file)), (std::vector<std::string>{"36@0+16/100", "38@0+16/70"}));
}

TEST(MidiFileReaderTest, FMI03_UnderstandsRunningStatusAndNoteOnWithVelocity0AsNoteOff)
{
    const auto file =
        header(0, 1, 96) +
        track(bytes({0x00, 0x99, 36, 100, 0x08, 36, 0, 0x08, 38, 90, 0x08, 38, 0}) + kEndOfTrack);

    EXPECT_EQ(notesOf(*readMidiFile(file)), (std::vector<std::string>{"36@0+8/100", "38@16+8/90"}));
}

TEST(MidiFileReaderTest, FMI03_SkipsEventsThatAreNoNotes)
{
    const auto file =
        header(0, 1, 96) + track(bytes({0x00, 0xF0, 0x03, 0x7E, 0x7F, 0xF7,        // system exclusive
                                        0x00, 0xF7, 0x01, 0x00,                    // escape
                                        0x00, 0xC9, 0x05,                          // program change
                                        0x00, 0xD9, 0x40,                          // channel pressure
                                        0x00, 0xB9, 0x07, 0x64,                    // controller
                                        0x00, 0xE9, 0x00, 0x40,                    // pitch bend
                                        0x00, 0xA9, 36,   0x20,                    // poly pressure
                                        0x00, 0xFF, 0x58, 0x04, 4,    2,    24, 8, // time signature
                                        0x04, 0x99, 36,   100,  0x04, 0x89, 36, 0}) +
                                 kEndOfTrack);

    EXPECT_EQ(notesOf(*readMidiFile(file)), std::vector<std::string>{"36@4+4/100"});
}

TEST(MidiFileReaderTest, FMI03_SkipsUnknownChunksAndExtraHeaderBytes)
{
    auto file =
        "MThd" + bigEndian(8, 4) + bigEndian(0, 2) + bigEndian(1, 2) + bigEndian(96, 2) + bytes({0, 0});
    file += "XFIH" + bigEndian(3, 4) + bytes({1, 2, 3});
    file += track(bytes({0x00, 0x99, 36, 100, 0x10, 0x89, 36, 0}) + kEndOfTrack);

    EXPECT_EQ(notesOf(*readMidiFile(file)), std::vector<std::string>{"36@0+16/100"});
}

TEST(MidiFileReaderTest, FMI03_ANoteOnPlaysUntilTheFirstNoteOffOfTheSameNoteAndChannel)
{
    const auto file =
        header(0, 1, 96) +
        track(bytes({0x00, 0x99, 36,   100,  0x04, 0x99, 36,   90,   0x04, 0x89, 38,   0,    0x00, 0x88,
                     36,   0,    0x00, 0x89, 36,   0,    0x04, 0x89, 36,   0,    0x04, 0x89, 36,   0}) +
              kEndOfTrack);

    EXPECT_EQ(notesOf(*readMidiFile(file)), (std::vector<std::string>{"36@0+8/100", "36@4+8/90"}));
}

TEST(MidiFileReaderTest, FMI03_ANoteThatIsNeverSwitchedOffEndsAtTheEndOfItsTrack)
{
    const auto file = header(0, 1, 96) + track(bytes({0x10, 0x99, 49, 110, 0x40, 0xFF, 0x2F, 0x00}));

    const auto content = readMidiFile(file);

    EXPECT_EQ(content->lengthTicks, 0x50);
    EXPECT_EQ(notesOf(*content), std::vector<std::string>{"49@16+64/110"});
}

TEST(MidiFileReaderTest, FMI03_ATrackWithoutEndOfTrackEndsAtItsLastEvent)
{
    const auto file = header(0, 1, 96) + track(bytes({0x00, 0x99, 36, 100, 0x20, 0x89, 36, 0}));

    EXPECT_EQ(readMidiFile(file)->lengthTicks, 0x20);
}

TEST(MidiFileReaderTest, FMI03_IgnoresWhatFollowsTheEndOfTrack)
{
    const auto file =
        header(0, 1, 96) + track(bytes({0x00, 0x99, 36, 100, 0x10, 0xFF, 0x2F, 0x00, 0x10, 0x89, 36, 0}));

    EXPECT_EQ(notesOf(*readMidiFile(file)), std::vector<std::string>{"36@0+16/100"});
}

TEST(MidiFileReaderTest, FMI03_ReadsLongDeltaTimes)
{
    const auto file = header(0, 1, 960) + track(bytes({0xFF, 0xFF, 0xFF, 0x7F, 0x99, 36, 100}) + kEndOfTrack);

    EXPECT_EQ(readMidiFile(file)->notes.at(0).startTick, 0x0FFFFFFF);
}

TEST(MidiFileReaderTest, FMI03_RejectsAFileThatIsNoMidiFile)
{
    EXPECT_FALSE(readMidiFile("").has_value());
    EXPECT_FALSE(
        readMidiFile("RIFF" + bigEndian(6, 4) + bigEndian(0, 4) + bigEndian(96, 2) + track(kEndOfTrack))
            .has_value());
    EXPECT_FALSE(readMidiFile("MThd" + bigEndian(4, 4) + bigEndian(0, 4)).has_value());
    EXPECT_FALSE(readMidiFile("MThd" + bigEndian(6, 4) + bigEndian(0, 4)).has_value());
}

TEST(MidiFileReaderTest, FMI03_RejectsType2SmpteAndZeroDivision)
{
    EXPECT_FALSE(readMidiFile(header(2, 1, 96) + track(kEndOfTrack)).has_value());
    EXPECT_FALSE(readMidiFile(header(0, 1, 0xE728) + track(kEndOfTrack)).has_value());
    EXPECT_FALSE(readMidiFile(header(0, 1, 0) + track(kEndOfTrack)).has_value());
}

TEST(MidiFileReaderTest, FMI03_RejectsTruncatedData)
{
    // Chunk longer than the file
    EXPECT_FALSE(
        readMidiFile(header(0, 1, 96) + "MTrk" + bigEndian(100, 4) + bytes({0x00, 0x99, 36})).has_value());
    // Meta text longer than the track
    EXPECT_FALSE(readMidiFile(header(0, 1, 96) + track(bytes({0x00, 0xFF, 0x03, 0x10, 'A'}))).has_value());
    // Chunk header cut off
    EXPECT_FALSE(readMidiFile(header(0, 1, 96) + "MTr").has_value());
}

TEST(MidiFileReaderTest, FMI03_RejectsInvalidEvents)
{
    // Data byte without a status before it
    EXPECT_FALSE(readMidiFile(header(0, 1, 96) + track(bytes({0x00, 36, 100}) + kEndOfTrack)).has_value());
    // System common message, not allowed in files
    EXPECT_FALSE(readMidiFile(header(0, 1, 96) + track(bytes({0x00, 0xF2, 0, 0}) + kEndOfTrack)).has_value());
    // Delta time longer than four bytes
    EXPECT_FALSE(readMidiFile(header(0, 1, 96) +
                              track(bytes({0x80, 0x80, 0x80, 0x80, 0x00, 0x99, 36, 100}) + kEndOfTrack))
                     .has_value());
}

TEST(MidiFileReaderTest, FMI03_RunningStatusEndsAtMetaAndSystemExclusiveEvents)
{
    EXPECT_FALSE(
        readMidiFile(header(0, 1, 96) +
                     track(bytes({0x00, 0x99, 36, 100, 0x00, 0xFF, 0x01, 0x00, 0x00, 36, 0}) + kEndOfTrack))
            .has_value());
    EXPECT_FALSE(
        readMidiFile(header(0, 1, 96) +
                     track(bytes({0x00, 0x99, 36, 100, 0x00, 0xF0, 0x01, 0xF7, 0x00, 36, 0}) + kEndOfTrack))
            .has_value());
}

} // namespace
} // namespace drumprog::io
