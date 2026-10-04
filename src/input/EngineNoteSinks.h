#pragma once

#include "engine/SampleEngine.h"
#include "input/INoteSink.h"

namespace drumprog::input
{

/// Computer keyboard hits from the GUI thread into the SampleEngine's GUI queue.
class GuiNoteSink final : public INoteSink
{
public:
    explicit GuiNoteSink(engine::SampleEngine& engine) : engine_(engine) {}

    bool noteOn(int midiNote, int velocity) noexcept override
    {
        return engine_.queueTrigger(midiNote, velocity);
    }

private:
    engine::SampleEngine& engine_;
};

/// MIDI hits from the MIDI thread into the SampleEngine's MIDI queue.
class MidiNoteSink final : public INoteSink
{
public:
    explicit MidiNoteSink(engine::SampleEngine& engine) : engine_(engine) {}

    bool noteOn(int midiNote, int velocity) noexcept override
    {
        return engine_.queueMidiTrigger(midiNote, velocity);
    }

private:
    engine::SampleEngine& engine_;
};

} // namespace drumprog::input
