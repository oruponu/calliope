#include "ui/window/OwnedWindow.h"

#import <Cocoa/Cocoa.h>

void keepAboveOwner(juce::Component& window, juce::Component& owner)
{
    auto* windowPeer = window.getPeer();
    auto* ownerPeer = owner.getTopLevelComponent()->getPeer();
    if (windowPeer == nullptr || ownerPeer == nullptr)
        return;

    // JUCE's native handle on macOS is the peer's NSView.
    NSWindow* child = [static_cast<NSView*>(windowPeer->getNativeHandle()) window];
    NSWindow* parent = [static_cast<NSView*>(ownerPeer->getNativeHandle()) window];
    if (child == nil || parent == nil || [child parentWindow] == parent)
        return;

    [parent addChildWindow:child ordered:NSWindowAbove];
}

void releaseFromOwner(juce::Component& window)
{
    auto* windowPeer = window.getPeer();
    if (windowPeer == nullptr)
        return;

    NSWindow* child = [static_cast<NSView*>(windowPeer->getNativeHandle()) window];
    if (child != nil && [child parentWindow] != nil)
        [[child parentWindow] removeChildWindow:child];
}
