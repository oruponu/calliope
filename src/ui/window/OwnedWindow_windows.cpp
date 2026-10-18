#include "ui/window/OwnedWindow.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // IWYU pragma: keep

void keepAboveOwner(juce::Component& window, juce::Component& owner)
{
    auto* windowPeer = window.getPeer();
    auto* ownerPeer = owner.getTopLevelComponent()->getPeer();
    if (windowPeer == nullptr || ownerPeer == nullptr)
        return;

    // GWLP_HWNDPARENT sets the owner of a top-level window, not its parent.
    SetWindowLongPtrW(static_cast<HWND>(windowPeer->getNativeHandle()), GWLP_HWNDPARENT,
                      reinterpret_cast<LONG_PTR>(ownerPeer->getNativeHandle()));
}

// A hidden owned window stays hidden when its owner comes to the front.
void releaseFromOwner(juce::Component&) {}
