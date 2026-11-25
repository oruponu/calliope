#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <set>

class Document;
class PlaybackEngine;
class TrackListComponent;

// Carries out the edits the track list asks for, through the undo history.
class TrackListActions
{
public:
    // Sets trackList's onChannelLabelClicked, onAddTrackRequested, onInsertTrackRequested,
    // onDuplicateTracksRequested, onRemoveTracksRequested, onMoveTracksRequested and onTrackRenamed.
    TrackListActions(TrackListComponent& trackList, Document& document, PlaybackEngine& playbackEngine);

private:
    void showChannelMenu(int trackIndex);
    void addTrack();
    void insertTrack(int trackIndex);
    void duplicateTracks(const std::set<int>& trackIndices);
    void removeTracks(const std::set<int>& trackIndices);
    void moveTracks(const std::set<int>& trackIndices, int gap);
    void renameTrack(int trackIndex, juce::String newName);

    TrackListComponent& trackList;
    Document& document;
    PlaybackEngine& playbackEngine;

    JUCE_DECLARE_NON_COPYABLE(TrackListActions)
};
