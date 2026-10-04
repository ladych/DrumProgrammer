#pragma once

namespace drumprog::input
{

/// Receives the note-ons of the live input. Each producer thread (GUI for the computer keyboard, MIDI
/// for the drum kit) has its own sink, because the queues to the audio thread are single-producer.
class INoteSink
{
public:
    virtual ~INoteSink() = default;

    /// False if the hit was dropped because the queue to the audio thread is full.
    virtual bool noteOn(int midiNote, int velocity) noexcept = 0;

protected:
    INoteSink() = default;
    INoteSink(const INoteSink&) = default;
    INoteSink(INoteSink&&) = default;
    INoteSink& operator=(const INoteSink&) = default;
    INoteSink& operator=(INoteSink&&) = default;
};

} // namespace drumprog::input
