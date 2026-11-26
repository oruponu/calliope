#include "app/SelectionLinks.h"
#include "ui/eventlist/EventListComponent.h"
#include "ui/pianoroll/PianoRollPanel.h"
#include "ui/tracklist/TrackListComponent.h"
#include <set>
#include <utility>

SelectionLinks::SelectionLinks(PianoRollPanel& pianoRollPanelRef, EventListComponent& eventListRef,
                               TrackListComponent& trackListRef, std::function<void()> trackSelectionChangedCallback)
    : pianoRollPanel(pianoRollPanelRef), eventList(eventListRef), trackList(trackListRef),
      trackSelectionChanged(std::move(trackSelectionChangedCallback))
{
    pianoRollPanel.getPianoRoll().onNoteSelectionChanged = [this](const auto& selected)
    {
        pianoRollPanel.getControllerLane().repaint();
        if (!selected.empty())
            pianoRollPanel.getControllerLane().clearPointSelection();
        if (updatingFromEventList)
            return;
        eventList.setSelectedNotes(selected);
    };
    pianoRollPanel.getPianoRoll().onSelectionTaken = [this]()
    { pianoRollPanel.getControllerLane().clearPointSelection(); };
    pianoRollPanel.getPianoRoll().onNoteDragPreviewChanged = [this](const auto& preview)
    { eventList.setNoteDragPreview(preview); };
    trackList.onTrackSelected = [this](int activeIdx, const std::set<int>& selected)
    {
        pianoRollPanel.getPianoRoll().setSelectedTracks(activeIdx, selected);
        pianoRollPanel.getControllerLane().setSelectedTracks(activeIdx, selected);
        eventList.setSelectedTracks(selected);
        trackSelectionChanged();
    };
    pianoRollPanel.getControllerLane().onNoteDragPreviewChanged = [this](const auto& preview)
    { eventList.setNoteDragPreview(preview); };
    pianoRollPanel.getControllerLane().onPointSelectionTaken = [this]()
    { pianoRollPanel.getPianoRoll().clearSelection(); };
    pianoRollPanel.getControllerLane().onPointSelectionChanged = [this](int trackIndex, const auto& events)
    {
        if (!updatingFromEventList)
            eventList.setSelectedEvents(trackIndex, events);
    };
    pianoRollPanel.getControllerLane().noteSelection = [this]()
    { return pianoRollPanel.getPianoRoll().getSelectedNotes(); };
    pianoRollPanel.getControllerLane().onNotesSelected = [this](const auto& notes)
    {
        pianoRollPanel.getPianoRoll().setSelectedNotes(notes);
        eventList.setSelectedNotes(pianoRollPanel.getPianoRoll().getSelectedNotes());
    };
    eventList.onNoteSelectionFromList = [this](const auto& noteRefs)
    {
        updatingFromEventList = true;
        pianoRollPanel.getControllerLane().clearPointSelection();
        pianoRollPanel.getPianoRoll().setSelectedNotes(noteRefs);
        updatingFromEventList = false;
        pianoRollPanel.getControllerLane().repaint();
    };
    eventList.onControllerSelectionFromList = [this](int trackIndex, const ControllerKind& kind, const auto& events)
    {
        // Switching the active track from a click in the list would surprise; other tracks' rows stay in the list.
        if (trackIndex != pianoRollPanel.getPianoRoll().getActiveTrackIndex())
            return;
        updatingFromEventList = true;
        pianoRollPanel.getControllerLane().showAndSelectPoints(kind, events);
        updatingFromEventList = false;
    };
}
