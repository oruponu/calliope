#include "ui/pianoroll/strips/ChordSymbolPainter.h"
#include "ui/theme/Theme.h"

namespace
{
float drawRun(juce::Graphics& g, const juce::Font& font, const std::string& text, float x, float baseline)
{
    if (text.empty())
        return x;

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, juce::String(text), x, baseline);
    glyphs.draw(g);
    return x + juce::GlyphArrangement::getStringWidth(font, juce::String(text));
}
} // namespace

void ChordSymbolPainter::draw(juce::Graphics& g, const ChordSymbolParts& parts, juce::Rectangle<int> area)
{
    using namespace calliope::theme;

    const auto mainFont = font::sans(font::sizeSM);
    const auto superFont = font::sans(font::size2XS);
    const float baseline = static_cast<float>(area.getCentreY()) + (mainFont.getAscent() - mainFont.getDescent()) / 2;
    // top-aligns the superscript with the root
    const float superBaseline = baseline - (mainFont.getAscent() - superFont.getAscent());

    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(area);

    float x = static_cast<float>(area.getX());
    x = drawRun(g, mainFont, parts.root, x, baseline);
    x = drawRun(g, superFont, parts.type, x, superBaseline);
    if (!parts.bass.empty())
        drawRun(g, mainFont, "/" + parts.bass, x, baseline);
}
