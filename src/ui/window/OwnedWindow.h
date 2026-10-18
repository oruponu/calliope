#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Makes `window` an owned window of the window holding `owner`: it stays above its owner and follows it behind
// other apps and into minimisation. On Linux this is only a hint, so following the minimisation is up to the
// window manager; on macOS the window also moves along with its owner. Both must already be on the desktop;
// calling it again is harmless.
void keepAboveOwner(juce::Component& window, juce::Component& owner);

// Detaches a hidden `window` from its owner so that it never comes back with the owner. Only macOS needs this.
void releaseFromOwner(juce::Component& window);
