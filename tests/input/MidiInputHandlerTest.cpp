#include "input/MidiInputHandler.h"

#include "input/FakeNoteSink.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <utility>

namespace drumprog::input
{
namespace
{

using Hit = std::pair<int, int>;

class MidiInputHandlerTest : public ::testing::Test
{
protected:
    template <std::size_t N> void receive(const std::array<std::uint8_t, N>& bytes)
    {
        handler.handleMessage(bytes);
    }

    FakeNoteSink sink;
    InputActivity activity;
    MidiInputHandler handler{sink, activity};
};

TEST_F(MidiInputHandlerTest, FIN03_NoteOnTriggersItsNoteWithVelocity)
{
    receive(std::array<std::uint8_t, 3>{0x99, 38, 87});
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{38, 87}}));
}

TEST_F(MidiInputHandlerTest, FIN03_NoteOnOfEveryChannelTriggers)
{
    receive(std::array<std::uint8_t, 3>{0x90, 36, 100});
    receive(std::array<std::uint8_t, 3>{0x9F, 42, 1});
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{36, 100}, {42, 1}}));
}

TEST_F(MidiInputHandlerTest, FIN04_NoteOnWithVelocityZeroIsANoteOffAndDoesNothing)
{
    receive(std::array<std::uint8_t, 3>{0x99, 38, 0});
    receive(std::array<std::uint8_t, 3>{0x89, 38, 64});
    EXPECT_TRUE(sink.hits.empty());
}

TEST_F(MidiInputHandlerTest, FIN03_OtherMessagesDoNotTrigger)
{
    receive(std::array<std::uint8_t, 3>{0xB9, 4, 90}); // hi-hat pedal controller
    receive(std::array<std::uint8_t, 2>{0x99, 38});    // truncated note-on
    handler.handleMessage({});
    EXPECT_TRUE(sink.hits.empty());
}

TEST_F(MidiInputHandlerTest, FIN05_IncomingMessagesSignalTheMidiLed)
{
    receive(std::array<std::uint8_t, 3>{0x99, 38, 87});
    receive(std::array<std::uint8_t, 3>{0x89, 38, 0});
    receive(std::array<std::uint8_t, 3>{0xB9, 4, 90});
    EXPECT_EQ(activity.midiCount(), 3U);
    EXPECT_EQ(activity.keyboardCount(), 0U);
}

TEST_F(MidiInputHandlerTest, FIN05_ClockAndActiveSensingDoNotBlinkTheLed)
{
    receive(std::array<std::uint8_t, 1>{0xF8});
    receive(std::array<std::uint8_t, 1>{0xFE});
    handler.handleMessage({});
    EXPECT_EQ(activity.midiCount(), 0U);
}

} // namespace
} // namespace drumprog::input
