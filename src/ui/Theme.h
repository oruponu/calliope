#pragma once

#include <juce_graphics/juce_graphics.h>

namespace calliope::theme
{
namespace surface
{
const juce::Colour bg{0xff0c0e13};
const juce::Colour bg2{0xff11141b};
const juce::Colour surface{0xff161a22};
const juce::Colour surface2{0xff1c2029};
const juce::Colour surface3{0xff232834};

const juce::Colour hover{juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.04f)};
const juce::Colour press{juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.07f)};
const juce::Colour selection{juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.12f)};
} // namespace surface

namespace border
{
const juce::Colour soft{0xff1c2029};
const juce::Colour normal{0xff232734};
const juce::Colour strong{0xff2e3344};
} // namespace border

namespace text
{
const juce::Colour t1{0xffe8e9ee};
const juce::Colour t2{0xffa3a7b2};
const juce::Colour t3{0xff6b6f7a};
const juce::Colour t4{0xff4b4f5a};
} // namespace text

namespace accent
{
const juce::Colour base{0xffe8ab3e};
const juce::Colour dim{0xffa17833};
const juce::Colour strong{0xffffc258};
const juce::Colour soft{juce::Colour(0xffe8ab3e).withAlpha(0.14f)};
} // namespace accent

namespace track
{
const juce::Colour blue{0xff55aee8};
const juce::Colour green{0xff5ec386};
const juce::Colour amber{0xffe1a447};
const juce::Colour rose{0xffeb8182};
const juce::Colour violet{0xffab93ed};
const juce::Colour teal{0xff25c2c2};
const juce::Colour lime{0xffa4c665};
const juce::Colour cyan{0xff35c5db};
const juce::Colour pink{0xffe189c5};
const juce::Colour sand{0xffc89164};
const juce::Colour sky{0xff63bfe1};

inline const juce::Array<juce::Colour>& palette()
{
    static const juce::Array<juce::Colour> p{blue, teal, violet, rose, pink, amber, sand, lime, cyan, green, sky};
    return p;
}
} // namespace track

namespace status
{
const juce::Colour ok{0xff54bf5c};
const juce::Colour warn{0xfff7a224};
const juce::Colour danger{0xfff14d4c};
} // namespace status

namespace font
{
constexpr float sizeXS = 10.0f;
constexpr float sizeSM = 11.0f;
constexpr float sizeMD = 12.0f;
constexpr float sizeLG = 12.5f;
constexpr float sizeXL = 14.0f;

inline juce::Font sans(float pt = sizeMD)
{
    return juce::Font(juce::FontOptions().withPointHeight(pt));
}

inline juce::Font mono(float pt = sizeMD)
{
    return juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withPointHeight(pt));
}
} // namespace font

namespace radius
{
constexpr float r1 = 3.0f;
constexpr float r2 = 5.0f;
constexpr float r3 = 7.0f;
constexpr float r4 = 10.0f;
constexpr float r5 = 14.0f;
} // namespace radius

namespace shadow
{
inline juce::DropShadow level1()
{
    return {juce::Colours::black.withAlpha(0.35f), 8, {0, 2}};
}
inline juce::DropShadow level2()
{
    return {juce::Colours::black.withAlpha(0.55f), 30, {0, 10}};
}
} // namespace shadow
} // namespace calliope::theme
