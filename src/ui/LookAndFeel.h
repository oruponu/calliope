#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace calliope
{
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    ~LookAndFeel() override = default;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive,
                           bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText, const juce::Drawable* icon,
                           const juce::Colour* textColour) override;

    void drawMenuBarItem(juce::Graphics&, int width, int height, int itemIndex, const juce::String& itemText,
                         bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar, juce::MenuBarComponent&) override;

    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical,
                       int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

private:
    void applyColourPalette();

    juce::Typeface::Ptr sansRegular;
    juce::Typeface::Ptr sansBold;
    juce::Typeface::Ptr mono;
    juce::Typeface::Ptr monoBold;
};
} // namespace calliope
