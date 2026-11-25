#include "ui/tracklist/TrackListActions.h"
#include "document/Document.h"
#include "edit/track/NewTrack.h"
#include "edit/track/TrackDuplication.h"
#include "engine/PlaybackEngine.h"
#include "model/TrackOrder.h"
#include "ui/tracklist/TrackListComponent.h"
#include "undo/track/ChannelChangeAction.h"
#include "undo/track/TrackAddAction.h"
#include "undo/track/TrackMoveAction.h"
#include "undo/track/TrackRemoveAction.h"
#include "undo/track/TrackRenameAction.h"
#include <ranges>
#include <set>
#include <utility>

TrackListActions::TrackListActions(TrackListComponent& trackListRef, Document& documentRef,
                                   PlaybackEngine& playbackEngineRef)
    : trackList(trackListRef), document(documentRef), playbackEngine(playbackEngineRef)
{
    trackList.onChannelLabelClicked = [this](int trackIndex) { showChannelMenu(trackIndex); };
    trackList.onAddTrackRequested = [this]() { addTrack(); };
    trackList.onInsertTrackRequested = [this](int trackIndex) { insertTrack(trackIndex); };
    trackList.onDuplicateTracksRequested = [this](const std::set<int>& trackIndices) { duplicateTracks(trackIndices); };
    trackList.onRemoveTracksRequested = [this](const std::set<int>& trackIndices) { removeTracks(trackIndices); };
    trackList.onMoveTracksRequested = [this](const std::set<int>& trackIndices, int gap)
    { moveTracks(trackIndices, gap); };
    trackList.onTrackRenamed = [this](int trackIndex, juce::String newName)
    { renameTrack(trackIndex, std::move(newName)); };
}

void TrackListActions::showChannelMenu(int trackIndex)
{
    int currentCh = document.getSequence().getTrack(trackIndex).getChannel();
    juce::PopupMenu menu;
    menu.addSectionHeader("Channel");
    for (int ch = 1; ch <= 16; ++ch)
    {
        menu.addItem(
            juce::String(ch), true, ch == currentCh,
            [this, trackIndex, ch]()
            {
                int currentChannel = document.getSequence().getTrack(trackIndex).getChannel();
                if (currentChannel == ch)
                    return;
                document.getHistory().beginNewTransaction();
                document.getHistory().perform(new ChannelChangeAction(
                    &document.getSequence(), trackIndex, currentChannel, ch, [this](int idx)
                    { playbackEngine.releaseActiveNotesForTrack(document.getSequence().getTrack(idx).getId()); }));
            });
    }
    menu.showMenuAsync(juce::PopupMenu::Options{});
}

void TrackListActions::addTrack()
{
    document.getHistory().beginNewTransaction("Add Track");
    auto& sequence = document.getSequence();
    const int index = sequence.getNumTracks();
    document.getHistory().perform(new TrackAddAction(&sequence, index, NewTrack::make(index)));
    trackList.refresh();
}

void TrackListActions::insertTrack(int trackIndex)
{
    document.getHistory().beginNewTransaction("Add Track");
    document.getHistory().perform(new TrackAddAction(&document.getSequence(), trackIndex, NewTrack::make(trackIndex)));
    trackList.refresh();
    trackList.setActiveTrackIndex(trackIndex);
}

void TrackListActions::duplicateTracks(const std::set<int>& trackIndices)
{
    auto& sequence = document.getSequence();
    document.getHistory().beginNewTransaction(trackIndices.size() > 1 ? "Duplicate Tracks" : "Duplicate Track");
    std::set<int> copies;
    for (int trackIndex : trackIndices)
    {
        const int sourceIndex = trackIndex + static_cast<int>(copies.size());
        document.getHistory().perform(
            new TrackAddAction(&sequence, sourceIndex + 1, TrackDuplication::makeCopy(sequence, sourceIndex)));
        copies.insert(sourceIndex + 1);
    }
    trackList.refresh();
    trackList.selectTracks(*copies.begin(), copies);
}

void TrackListActions::removeTracks(const std::set<int>& trackIndices)
{
    auto& sequence = document.getSequence();
    if (trackIndices.empty() || std::cmp_greater_equal(trackIndices.size(), sequence.getNumTracks()))
        return;

    document.getHistory().beginNewTransaction("Remove Track");
    for (int trackIndex : trackIndices | std::views::reverse)
        document.getHistory().perform(new TrackRemoveAction(&sequence, trackIndex));

    int newActive = juce::jlimit(0, sequence.getNumTracks() - 1, *trackIndices.begin());
    trackList.refresh();
    trackList.setActiveTrackIndex(newActive);
}

void TrackListActions::moveTracks(const std::set<int>& trackIndices, int gap)
{
    auto& sequence = document.getSequence();
    auto order = TrackOrder::moveTogether(sequence.getNumTracks(), trackIndices, gap);
    if (TrackOrder::isIdentity(order))
        return;

    const auto newIndexOf = TrackOrder::inverse(order);
    const int newActive = newIndexOf[trackList.getActiveTrackIndex()];
    const auto moved = trackIndices | std::views::transform([&](int index) { return newIndexOf[index]; }) |
                       std::ranges::to<std::set>();

    document.getHistory().beginNewTransaction(trackIndices.size() > 1 ? "Move Tracks" : "Move Track");
    document.getHistory().perform(new TrackMoveAction(&sequence, std::move(order)));
    trackList.refresh();
    trackList.selectTracks(newActive, moved);
}

void TrackListActions::renameTrack(int trackIndex, juce::String newName)
{
    if (trackIndex < 0 || trackIndex >= document.getSequence().getNumTracks())
        return;
    std::string oldName = document.getSequence().getTrack(trackIndex).getName();
    std::string requested = newName.toStdString();
    if (oldName == requested)
        return;
    document.getHistory().beginNewTransaction();
    document.getHistory().perform(new TrackRenameAction(&document.getSequence(), trackIndex, oldName, requested));
}
