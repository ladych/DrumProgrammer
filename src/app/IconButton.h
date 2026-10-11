#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <memory>
#include <optional>

namespace drumprog::app
{

/// The icons of resources/Icons (Lucide Icons, AP14).
enum class Icon : std::uint8_t
{
    rewind,
    play,
    stop,
    loop,
    record,
    patternMode,
    songMode,
    draw,
    select,
    erase,
    overdub,
    replace,
    zoomIn,
    zoomOut,
    add,
};

/// Humble object (AP14): a button that shows only an icon; what it does says its tooltip. A switched-on
/// toggle button is drawn in the accent colour; setHighlight fills it with a state colour instead (running
/// playback, armed recording).
class IconButton final : public juce::Button
{
public:
    IconButton(Icon icon, const juce::String& tooltip);

    /// Fills the button with colour, nullopt draws it normally again.
    void setHighlight(std::optional<juce::Colour> colour);

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override;

private:
    struct Look
    {
        juce::Colour background;
        juce::Colour outline;
        juce::Colour icon;
    };

    [[nodiscard]] Look lookOf(bool highlighted, bool down) const;
    void colourIcon(juce::Colour colour);

    std::unique_ptr<juce::Drawable> drawable_;
    juce::Colour iconColour_;
    std::optional<juce::Colour> highlight_;

    JUCE_LEAK_DETECTOR(IconButton)
};

} // namespace drumprog::app
