#include "app/JuceBackingTrackLoader.h"

#include <array>
#include <string>

namespace drumprog::app
{
namespace
{

/// IAudioFileStream on top of a BufferingAudioReader.
class BufferedStream final : public engine::IAudioFileStream
{
public:
    explicit BufferedStream(std::unique_ptr<juce::BufferingAudioReader> reader)
        : reader_(std::move(reader)), mono_(reader_->numChannels == 1)
    {
        // Never block the audio thread: frames not read yet play as silence.
        reader_->setReadTimeout(0);
    }

    [[nodiscard]] double sampleRate() const noexcept override { return reader_->sampleRate; }
    [[nodiscard]] std::int64_t lengthInSamples() const noexcept override { return reader_->lengthInSamples; }

    void read(std::int64_t start, int numFrames, float* left, float* right) noexcept override
    {
        // Frames outside the file or not buffered yet come back as zeros.
        const std::array<float*, 2> channels{left, right};
        reader_->read(channels.data(), mono_ ? 1 : 2, start, numFrames);
        if (mono_)
            juce::FloatVectorOperations::copy(right, left, numFrames);
    }

private:
    std::unique_ptr<juce::BufferingAudioReader> reader_;
    bool mono_;
};

juce::File toJuceFile(const std::filesystem::path& file)
{
    const auto utf8 = file.u8string();
    return juce::File{juce::String::fromUTF8(std::string{utf8.begin(), utf8.end()}.c_str())};
}

} // namespace

JuceBackingTrackLoader::JuceBackingTrackLoader()
{
    // Pflichtenheft chapter 3: samples and backing track are WAV (16/24/32-bit float, mono/stereo).
    formats_.registerFormat(new juce::WavAudioFormat(), true);
    readThread_.startThread();
}

JuceBackingTrackLoader::~JuceBackingTrackLoader()
{
    readThread_.stopThread(1000);
}

std::unique_ptr<engine::IAudioFileStream> JuceBackingTrackLoader::open(const std::filesystem::path& file)
{
    auto* reader = formats_.createReaderFor(toJuceFile(file));
    if (reader == nullptr)
        return nullptr;
    if (reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
    {
        delete reader;
        return nullptr;
    }
    const auto bufferFrames = static_cast<int>(reader->sampleRate * kBufferSeconds);
    return std::make_unique<BufferedStream>(
        std::make_unique<juce::BufferingAudioReader>(reader, readThread_, bufferFrames));
}

juce::String JuceBackingTrackLoader::wildcardPattern() const
{
    return formats_.getWildcardForAllFormats();
}

} // namespace drumprog::app
