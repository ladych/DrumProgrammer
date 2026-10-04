#pragma once

#include "engine/ISampleLoader.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace drumprog::app
{

/// Humble object (E-03): decodes sample files with JUCE's AudioFormatManager (F-SE-01).
class JuceSampleLoader final : public engine::ISampleLoader
{
public:
    JuceSampleLoader();

    [[nodiscard]] std::optional<engine::SampleBuffer> load(const std::filesystem::path& file) override;

    /// File pattern for the file chooser, e.g. "*.wav".
    [[nodiscard]] juce::String wildcardPattern() const;

private:
    juce::AudioFormatManager formats_;
};

} // namespace drumprog::app
