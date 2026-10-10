#pragma once

#include <cstdint>

namespace drumprog::engine
{

/// An audio file read from the disk while it plays (F-BT-02), for the backing track. The GUI thread opens
/// it; afterwards only the audio thread reads from it. Implementations read ahead on a thread of their own
/// and must never block the audio thread for long.
class IAudioFileStream
{
public:
    IAudioFileStream() = default;
    virtual ~IAudioFileStream() = default;
    IAudioFileStream(const IAudioFileStream&) = delete;
    IAudioFileStream& operator=(const IAudioFileStream&) = delete;
    IAudioFileStream(IAudioFileStream&&) = delete;
    IAudioFileStream& operator=(IAudioFileStream&&) = delete;

    [[nodiscard]] virtual double sampleRate() const noexcept = 0;
    [[nodiscard]] virtual std::int64_t lengthInSamples() const noexcept = 0;
    /// Fills numFrames frames from the file frame start on into left and right; frames outside the file,
    /// and frames not read from the disk yet, are silence. A mono file plays on both channels.
    virtual void read(std::int64_t start, int numFrames, float* left, float* right) noexcept = 0;
};

} // namespace drumprog::engine
