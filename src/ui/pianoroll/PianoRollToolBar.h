#pragma once

#include "model/grid/GridDivision.h"
#include "ui/pianoroll/PianoRollComponent.h"
#include "ui/widgets/ToolButton.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class PianoRollToolBar : public juce::Component
{
public:
    PianoRollToolBar(); // reads snapEnabled from the settings; grid 1/16, Select tool

    void setActiveTool(PianoRollComponent::EditMode mode);
    bool isSnapEnabled() const;
    // Also saves the setting and calls onSnapChanged.
    void toggleSnap();
    GridDivision getGridDivision() const;
    // Whether focused is one of the buttons or the grid box, or inside one.
    bool contains(const juce::Component* focused) const;

    std::function<void(PianoRollComponent::EditMode)> onToolClicked;
    std::function<void(bool enabled)> onSnapChanged;
    std::function<void(GridDivision)> onGridDivisionChanged;
    std::function<void()> onBackgroundClicked;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    static constexpr int height = 32;

private:
    ToolButton editToolButton{ToolButton::EditTool};
    ToolButton selectToolButton{ToolButton::SelectTool};
    ToolButton snapButton{ToolButton::Snap};
    juce::ComboBox quantizeComboBox;
    int separatorX = 0;

    JUCE_DECLARE_NON_COPYABLE(PianoRollToolBar)
};
