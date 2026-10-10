#pragma once

#include "ui/IBackingTrackLoader.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace drumprog::app
{

/// Humble object (E-03): opens WAV files for streaming (F-BT-02). A juce::BufferingAudioReader reads some
/// seconds ahead of the playback on a background thread, so the audio thread only copies from memory and
/// gets silence instead of waiting where the disk is too slow.
class JuceBackingTrackLoader final : public ui::IBackingTrackLoader
{
public:
    static constexpr int kBufferSeconds = 10;

    JuceBackingTrackLoader();
    ~JuceBackingTrackLoader() override;

    JuceBackingTrackLoader(const JuceBackingTrackLoader&) = delete;
    JuceBackingTrackLoader& operator=(const JuceBackingTrackLoader&) = delete;
    JuceBackingTrackLoader(JuceBackingTrackLoader&&) = delete;
    JuceBackingTrackLoader& operator=(JuceBackingTrackLoader&&) = delete;

    [[nodiscard]] std::unique_ptr<engine::IAudioFileStream> open(const std::filesystem::path& file) override;
    /// For the waveform display (AudioThumbnail).
    [[nodiscard]] juce::AudioFormatManager& formats() noexcept { return formats_; }
    /// File pattern for the file chooser, e.g. "*.wav".
    [[nodiscard]] juce::String wildcardPattern() const;

private:
    juce::AudioFormatManager formats_;
    juce::TimeSliceThread readThread_{"Backing-Track"};
};

} // namespace drumprog::app
