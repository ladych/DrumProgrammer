#include "app/IconButton.h"

#include "app/Theme.h"

#include <IconData.h>

#include <algorithm>
#include <array>
#include <cstddef>

namespace drumprog::app
{
namespace
{

constexpr float kCornerSize = 4.0F;
constexpr int kIconInset = 6;

struct SvgData
{
    const char* data;
    int size;
};

SvgData svgOf(Icon icon)
{
    // In the order of enum class Icon.
    static const std::array<SvgData, 15> svgs{{{IconData::skipback_svg, IconData::skipback_svgSize},
                                               {IconData::play_svg, IconData::play_svgSize},
                                               {IconData::square_svg, IconData::square_svgSize},
                                               {IconData::repeat_svg, IconData::repeat_svgSize},
                                               {IconData::circle_svg, IconData::circle_svgSize},
                                               {IconData::grid3x3_svg, IconData::grid3x3_svgSize},
                                               {IconData::listmusic_svg, IconData::listmusic_svgSize},
                                               {IconData::pencil_svg, IconData::pencil_svgSize},
                                               {IconData::mousepointer2_svg, IconData::mousepointer2_svgSize},
                                               {IconData::eraser_svg, IconData::eraser_svgSize},
                                               {IconData::layers_svg, IconData::layers_svgSize},
                                               {IconData::replace_svg, IconData::replace_svgSize},
                                               {IconData::zoomin_svg, IconData::zoomin_svgSize},
                                               {IconData::zoomout_svg, IconData::zoomout_svgSize},
                                               {IconData::plus_svg, IconData::plus_svgSize}}};
    return svgs.at(static_cast<std::size_t>(icon));
}

/// Lucide draws with stroke="currentColor", which the JUCE SVG parser does not know: the icon is loaded in
/// white and coloured afterwards.
std::unique_ptr<juce::Drawable> loadIcon(Icon icon)
{
    const auto svg = svgOf(icon);
    const auto text = juce::String::fromUTF8(svg.data, svg.size).replace("currentColor", "#ffffff");
    const auto xml = juce::parseXML(text);
    return xml != nullptr ? juce::Drawable::createFromSVG(*xml) : nullptr;
}

} // namespace

IconButton::IconButton(Icon icon, const juce::String& tooltip)
    : juce::Button(tooltip), drawable_(loadIcon(icon)), iconColour_(juce::Colours::white)
{
    setTooltip(tooltip);
    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);
}

void IconButton::setHighlight(std::optional<juce::Colour> colour)
{
    if (highlight_ == colour)
        return;
    highlight_ = colour;
    repaint();
}

IconButton::Look IconButton::lookOf(bool highlighted, bool down) const
{
    Look look{.background = theme::kWidget, .outline = theme::kOutline, .icon = theme::kText};
    if (highlight_)
        look = {
            .background = *highlight_, .outline = highlight_->brighter(0.2F), .icon = juce::Colours::white};
    else if (getToggleState())
        look = {.background = theme::kWidget.overlaidWith(theme::kAccent.withAlpha(0.18F)),
                .outline = theme::kAccent,
                .icon = theme::kAccent};
    if (down)
        look.background = look.background.darker(0.2F);
    else if (highlighted)
        look.background = look.background.brighter(0.15F);
    if (!isEnabled())
        look.icon = look.icon.withMultipliedAlpha(0.35F);
    return look;
}

void IconButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const auto area = getLocalBounds().toFloat().reduced(0.5F);
    const auto look = lookOf(highlighted, down);
    g.setColour(look.background);
    g.fillRoundedRectangle(area, kCornerSize);
    g.setColour(look.outline);
    g.drawRoundedRectangle(area, kCornerSize, 1.0F);
    if (drawable_ == nullptr)
        return;
    colourIcon(look.icon);
    const auto side = static_cast<float>(std::min(getWidth(), getHeight()) - 2 * kIconInset);
    drawable_->drawWithin(g,
                          area.withSizeKeepingCentre(side, side),
                          juce::RectanglePlacement::centred,
                          look.icon.getFloatAlpha());
}

void IconButton::colourIcon(juce::Colour colour)
{
    const auto opaque = colour.withAlpha(1.0F);
    if (opaque == iconColour_)
        return;
    drawable_->replaceColour(iconColour_, opaque);
    iconColour_ = opaque;
}

} // namespace drumprog::app
