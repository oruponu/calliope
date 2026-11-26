#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class EventListComponent;
class PianoRollPanel;
class PlaybackEngine;
class TransportBarComponent;

// Keeps the playhead and the loop region in step across the piano roll panel, the event list and the transport bar,
// and follows the playhead while playing.
class PlayheadLinks
{
public:
    // Sets the piano roll's onPlayheadMoved and onLoopRegionChanged, the event list's onEventSelected and the
    // transport bar's onPlayheadMoved, onScrollToPlayhead, onReturnToStart, onPlaybackStateChanged and
    // onLoopRegionChanged.
    PlayheadLinks(PianoRollPanel& pianoRollPanel, EventListComponent& eventList, TransportBarComponent& transportBar,
                  PlaybackEngine& playbackEngine, juce::Component& vblankOwner);

    void stopFollowing();

private:
    void onVBlank();

    PianoRollPanel& pianoRollPanel;
    EventListComponent& eventList;
    TransportBarComponent& transportBar;
    PlaybackEngine& playbackEngine;
    juce::Component& vblankOwner;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;

    JUCE_DECLARE_NON_COPYABLE(PlayheadLinks)
};
