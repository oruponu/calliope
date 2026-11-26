#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class EventListComponent;
class PianoRollPanel;
class TrackListComponent;

// Keeps the selected tracks, notes and controller points in step across the track list, the piano roll panel and the
// event list, and passes on the note drag previews.
class SelectionLinks
{
public:
    // trackSelectionChanged: called last when the track list's selection changes.
    SelectionLinks(PianoRollPanel& pianoRollPanel, EventListComponent& eventList, TrackListComponent& trackList,
                   std::function<void()> trackSelectionChanged);

private:
    PianoRollPanel& pianoRollPanel;
    EventListComponent& eventList;
    TrackListComponent& trackList;
    std::function<void()> trackSelectionChanged;
    bool updatingFromEventList = false;

    JUCE_DECLARE_NON_COPYABLE(SelectionLinks)
};
