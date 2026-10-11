#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app::theme
{

// Colours of the main window after the GUI draft (docs/GUI-Entwurf_v0.1.png, AP14).

/// Gaps between the areas and the status line: darker than the areas, so they stand apart.
inline const juce::Colour kGap{0xFF121418};
/// Background of an area (top bar, pattern list, song arrangement, piano roll, right column).
inline const juce::Colour kPanel{0xFF1E2227};
/// Widgets inside an area: buttons, combo boxes, text fields.
inline const juce::Colour kWidget{0xFF2A2F36};
inline const juce::Colour kOutline{0xFF3A4048};
inline const juce::Colour kText{0xFFD6D9DD};
inline const juce::Colour kTextDim{0xFF8A919A};
/// Active state: switched-on tool, running playback, chosen mode.
inline const juce::Colour kAccent{0xFFF59E0B};
inline const juce::Colour kRecord{0xFFD03030};
/// Song position and playback cursor.
inline const juce::Colour kPosition{0xFF22C55E};
inline const juce::Colour kPositionBackground{0xFF101317};
inline const juce::Colour kSliderFill{0xFF38A8E0};

/// JUCE look and feel with the colours above.
class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
        : juce::LookAndFeel_V4(ColourScheme{kPanel,
                                            kWidget,
                                            kPanel,
                                            kOutline,
                                            kText,
                                            kSliderFill,
                                            juce::Colours::white,
                                            juce::Colour{0xFF2B5C8A},
                                            kText})
    {
        setColour(juce::TooltipWindow::backgroundColourId, kWidget);
        setColour(juce::TooltipWindow::textColourId, kText);
        setColour(juce::TooltipWindow::outlineColourId, kOutline);
    }
};

} // namespace drumprog::app::theme
