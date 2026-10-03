#include "app/JuceSampleLoader.h"

#include <limits>
#include <memory>
#include <string>

namespace drumprog::app
{
namespace
{

juce::File toJuceFile(const std::filesystem::path& file)
{
    const auto utf8 = file.u8string();
    return juce::File{juce::String::fromUTF8(std::string{utf8.begin(), utf8.end()}.c_str())};
}

} // namespace

JuceSampleLoader::JuceSampleLoader()
{
    // Supported formats are registered here only; AIFF/FLAC (Kann) need one line each.
    formats_.registerFormat(new juce::WavAudioFormat(), true);
}

std::optional<engine::SampleBuffer> JuceSampleLoader::load(const std::filesystem::path& file)
{
    const std::unique_ptr<juce::AudioFormatReader> reader(formats_.createReaderFor(toJuceFile(file)));
    if (reader == nullptr || reader->lengthInSamples <= 0 ||
        reader->lengthInSamples > std::numeric_limits<int>::max())
        return std::nullopt;

    const auto frames = static_cast<int>(reader->lengthInSamples);
    const auto numChannels = static_cast<int>(reader->numChannels);
    juce::AudioBuffer<float> buffer(numChannels, frames);
    if (!reader->read(&buffer, 0, frames, 0, true, true))
        return std::nullopt;

    engine::SampleBuffer sample{reader->sampleRate, {}};
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* data = buffer.getReadPointer(channel);
        sample.channels.emplace_back(data, data + frames);
    }
    return sample;
}

juce::String JuceSampleLoader::wildcardPattern() const
{
    return formats_.getWildcardForAllFormats();
}

} // namespace drumprog::app
