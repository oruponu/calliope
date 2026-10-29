#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

// Leaves the keyboard focus where it is when Tab or Shift+Tab is pressed.
class NoTabFocusTraverser : public juce::ComponentTraverser
{
public:
    juce::Component* getDefaultComponent(juce::Component* parentComponent) override
    {
        return defaultTraverser.getDefaultComponent(parentComponent);
    }

    juce::Component* getNextComponent(juce::Component*) override { return nullptr; }
    juce::Component* getPreviousComponent(juce::Component*) override { return nullptr; }

    // Empty because Tab falls back to wrapping round to the first or last of these.
    std::vector<juce::Component*> getAllComponents(juce::Component*) override { return {}; }

private:
    juce::KeyboardFocusTraverser defaultTraverser;
};
