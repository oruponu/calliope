#include "LookAndFeel.h"
#include "Theme.h"
#include "BinaryData.h"

namespace calliope
{
using namespace juce;
namespace t = theme;

LookAndFeel::LookAndFeel()
{
    sansRegular =
        Typeface::createSystemTypefaceFor(BinaryData::NotoSansRegular_ttf, BinaryData::NotoSansRegular_ttfSize);
    sansBold = Typeface::createSystemTypefaceFor(BinaryData::NotoSansBold_ttf, BinaryData::NotoSansBold_ttfSize);
    mono =
        Typeface::createSystemTypefaceFor(BinaryData::InconsolataRegular_ttf, BinaryData::InconsolataRegular_ttfSize);
    monoBold = Typeface::createSystemTypefaceFor(BinaryData::InconsolataBold_ttf, BinaryData::InconsolataBold_ttfSize);

    applyColourPalette();
}

void LookAndFeel::applyColourPalette()
{
    setColour(ResizableWindow::backgroundColourId, t::surface::bg);
    setColour(DocumentWindow::textColourId, t::text::t1);

    setColour(Label::textColourId, t::text::t1);
    setColour(Label::backgroundColourId, Colours::transparentBlack);

    setColour(TextButton::buttonColourId, t::surface::surface2);
    setColour(TextButton::buttonOnColourId, t::accent::soft);
    setColour(TextButton::textColourOffId, t::text::t1);
    setColour(TextButton::textColourOnId, t::accent::strong);

    setColour(ToggleButton::textColourId, t::text::t2);
    setColour(ToggleButton::tickColourId, t::accent::base);
    setColour(ToggleButton::tickDisabledColourId, t::text::t4);

    setColour(ComboBox::backgroundColourId, t::surface::surface2);
    setColour(ComboBox::outlineColourId, t::border::normal);
    setColour(ComboBox::textColourId, t::text::t1);
    setColour(ComboBox::arrowColourId, t::text::t3);
    setColour(ComboBox::focusedOutlineColourId, t::accent::dim);

    setColour(PopupMenu::backgroundColourId, t::surface::surface);
    setColour(PopupMenu::textColourId, t::text::t1);
    setColour(PopupMenu::headerTextColourId, t::text::t3);
    setColour(PopupMenu::highlightedBackgroundColourId, t::surface::selection);
    setColour(PopupMenu::highlightedTextColourId, t::text::t1);

    setColour(TextEditor::backgroundColourId, t::surface::surface2);
    setColour(TextEditor::textColourId, t::text::t1);
    setColour(TextEditor::highlightColourId, t::accent::soft);
    setColour(TextEditor::highlightedTextColourId, t::accent::strong);
    setColour(TextEditor::outlineColourId, t::border::normal);
    setColour(TextEditor::focusedOutlineColourId, t::accent::dim);
    setColour(TextEditor::shadowColourId, Colours::transparentBlack);

    setColour(Slider::backgroundColourId, t::surface::surface3);
    setColour(Slider::trackColourId, t::accent::dim);
    setColour(Slider::thumbColourId, t::accent::base);
    setColour(Slider::rotarySliderFillColourId, t::accent::base);
    setColour(Slider::rotarySliderOutlineColourId, t::border::strong);
    setColour(Slider::textBoxTextColourId, t::text::t1);
    setColour(Slider::textBoxBackgroundColourId, t::surface::surface);
    setColour(Slider::textBoxOutlineColourId, t::border::normal);

    setColour(ScrollBar::backgroundColourId, Colours::transparentBlack);
    setColour(ScrollBar::thumbColourId, t::surface::surface3);
    setColour(ScrollBar::trackColourId, t::surface::bg2);

    setColour(ListBox::backgroundColourId, t::surface::surface);
    setColour(ListBox::textColourId, t::text::t1);
    setColour(ListBox::outlineColourId, t::border::normal);

    setColour(TooltipWindow::backgroundColourId, t::surface::surface3);
    setColour(TooltipWindow::textColourId, t::text::t1);
    setColour(TooltipWindow::outlineColourId, t::border::normal);
}

Typeface::Ptr LookAndFeel::getTypefaceForFont(const Font& font)
{
    const auto name = font.getTypefaceName();

    if (name == Font::getDefaultMonospacedFontName())
        return font.isBold() ? monoBold : mono;

    if (name == Font::getDefaultSansSerifFontName())
        return font.isBold() ? sansBold : sansRegular;

    return LookAndFeel_V4::getTypefaceForFont(font);
}

Font LookAndFeel::getTextButtonFont(TextButton&, int)
{
    return t::font::sans(t::font::sizeMD);
}

Font LookAndFeel::getLabelFont(Label& label)
{
    return label.getFont();
}

Font LookAndFeel::getPopupMenuFont()
{
    return t::font::sans(t::font::sizeMD);
}

void LookAndFeel::drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour, bool isMouseOver,
                                       bool isMouseDown)
{
    auto bounds = b.getLocalBounds().toFloat().reduced(0.5f);
    const float r = t::radius::r2;

    Colour fill = backgroundColour;
    if (b.getToggleState())
        fill = t::accent::soft;
    else if (isMouseDown)
        fill = t::surface::surface3.overlaidWith(t::surface::press);
    else if (isMouseOver)
        fill = t::surface::surface3;

    Colour stroke = b.getToggleState() ? t::accent::dim : t::border::normal;
    if (isMouseOver && !b.getToggleState())
        stroke = t::border::strong;

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, r);

    g.setColour(stroke);
    g.drawRoundedRectangle(bounds, r, 1.0f);
}

void LookAndFeel::drawPopupMenuBackground(Graphics& g, int w, int h)
{
    g.fillAll(t::surface::surface);

    g.setColour(t::border::normal);
    g.drawRect(0, 0, w, h, 1);
}

void LookAndFeel::getIdealPopupMenuItemSize(const String& text, bool isSeparator, int /*standardMenuItemHeight*/,
                                            int& idealWidth, int& idealHeight)
{
    constexpr int itemHeight = 25;

    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = 9;
        return;
    }

    idealHeight = itemHeight;
    idealWidth = GlyphArrangement::getStringWidthInt(getPopupMenuFont(), text) + itemHeight * 2;
}

void LookAndFeel::drawPopupMenuItem(Graphics& g, const Rectangle<int>& area, bool isSeparator, bool isActive,
                                    bool isHighlighted, bool isTicked, bool hasSubMenu, const String& text,
                                    const String& shortcutKeyText, const Drawable*, const Colour*)
{
    if (isSeparator)
    {
        auto sep = area.reduced(8, 0).toFloat();
        sep.setHeight(1.0f);
        sep.setY(area.getCentreY() - 0.5f);
        g.setColour(t::border::soft);
        g.fillRect(sep);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(t::surface::selection);
        g.fillRoundedRectangle(area.reduced(4, 1).toFloat(), t::radius::r1);
    }

    Colour fg = !isActive ? t::text::t4 : t::text::t1;

    g.setColour(fg);
    g.setFont(getPopupMenuFont());

    auto textArea = area.reduced(12, 0).withTrimmedLeft(12);
    if (hasSubMenu)
        textArea = textArea.withTrimmedRight(12);
    g.drawFittedText(text, textArea, Justification::centredLeft, 1);

    if (hasSubMenu)
    {
        const float backX = (float)area.getRight() - 18.0f;
        const float tipX = (float)area.getRight() - 12.0f;
        const float ah = (float)area.getHeight() * 0.4f;
        const float cy = (float)area.getCentreY();

        Path arrow;
        arrow.startNewSubPath(backX, cy - ah * 0.5f);
        arrow.lineTo(tipX, cy);
        arrow.lineTo(backX, cy + ah * 0.5f);

        g.setColour(fg.withMultipliedAlpha(0.7f));
        g.strokePath(arrow, PathStrokeType(1.5f));
    }
    else if (shortcutKeyText.isNotEmpty())
    {
        g.setColour(t::text::t3);
        g.setFont(t::font::mono(t::font::sizeXS));
        g.drawFittedText(shortcutKeyText, textArea, Justification::centredRight, 1);
    }

    if (isTicked)
    {
        g.setColour(t::accent::base);
        const int s = 6;
        g.fillEllipse((float)area.getX() + 12, (float)area.getCentreY() - s / 2.f, (float)s, (float)s);
    }
}

Font LookAndFeel::getMenuBarFont(MenuBarComponent&, int, const String&)
{
    return t::font::sans(t::font::sizeLG);
}

void LookAndFeel::drawMenuBarItem(Graphics& g, int width, int height, int itemIndex, const String& itemText,
                                  bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar, MenuBarComponent& menuBar)
{
    ignoreUnused(itemIndex, isMouseOverBar);

    if (isMenuOpen || isMouseOverItem)
    {
        g.setColour(t::surface::selection);
        g.fillRoundedRectangle(Rectangle<int>(0, 0, width, height).reduced(2).toFloat(), t::radius::r1);
    }

    g.setColour(menuBar.isEnabled() ? t::text::t1 : t::text::t4);
    g.setFont(getMenuBarFont(menuBar, itemIndex, itemText));
    g.drawFittedText(itemText, 0, 0, width, height, Justification::centred, 1);
}

void LookAndFeel::drawScrollbar(Graphics& g, ScrollBar&, int x, int y, int width, int height, bool isVertical,
                                int thumbStartPos, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    Rectangle<int> thumb = isVertical ? Rectangle<int>(x + 2, thumbStartPos, width - 4, thumbSize)
                                      : Rectangle<int>(thumbStartPos, y + 2, thumbSize, height - 4);

    Colour c = t::surface::surface3;
    if (isMouseDown)
        c = t::accent::dim;
    else if (isMouseOver)
        c = t::border::strong;

    g.setColour(c);
    g.fillRoundedRectangle(thumb.toFloat(),
                           thumb.getWidth() < thumb.getHeight() ? thumb.getWidth() / 2.0f : thumb.getHeight() / 2.0f);
}
} // namespace calliope
