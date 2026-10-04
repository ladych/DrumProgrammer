#pragma once

#include "engine/Clock.h"
#include "engine/SampleEngine.h"
#include "input/INoteSink.h"

namespace drumprog::input
{

/// Computer keyboard hits from the GUI thread into the SampleEngine's GUI queue, stamped with the time
/// they were played (F-IN-08).
class GuiNoteSink final : public INoteSink
{
public:
    GuiNoteSink(engine::SampleEngine& engine, engine::Clock clock) : engine_(engine), clock_(clock) {}

    bool noteOn(int midiNote, int velocity) noexcept override
    {
        return engine_.queueLiveTrigger(midiNote, velocity, clock_());
    }

private:
    engine::SampleEngine& engine_;
    engine::Clock clock_;
};

/// MIDI hits from the MIDI thread into the SampleEngine's MIDI queue, stamped with the time they
/// arrived (F-IN-08).
class MidiNoteSink final : public INoteSink
{
public:
    MidiNoteSink(engine::SampleEngine& engine, engine::Clock clock) : engine_(engine), clock_(clock) {}

    bool noteOn(int midiNote, int velocity) noexcept override
    {
        return engine_.queueMidiTrigger(midiNote, velocity, clock_());
    }

private:
    engine::SampleEngine& engine_;
    engine::Clock clock_;
};

} // namespace drumprog::input
