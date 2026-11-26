#pragma once

#include "ui/widgets/NoTabFocusTraverser.h"

// For a panel that is not a focus target itself: a click on a part of it that takes no focus passes the focus up
// to the panel's parent, as if the panel were not there. Tab leaves the focus where it is.
class PassFocusUpTraverser : public NoTabFocusTraverser
{
public:
    juce::Component* getDefaultComponent(juce::Component*) override { return nullptr; }
};
