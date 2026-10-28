#pragma once

#include "model/MidiSequence.h"
#include "ui/TrackNameEditor.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include <set>

class TrackListComponent : public juce::Component, public MidiSequence::Listener
{
public:
    ~TrackListComponent() override;

    void setSequence(MidiSequence* seq);
    void refresh();

    int getActiveTrackIndex() const;
    void setActiveTrackIndex(int index);
    void selectTracks(int activeIndex, const std::set<int>& indices);
    const std::set<int>& getSelectedTrackIndices() const;
    bool isTrackSelected(int index) const;
    void setSelectedTrackIndices(const std::set<int>& indices);

    std::function<void(int activeIndex, const std::set<int>& selectedIndices)> onTrackSelected;
    std::function<juce::String(int trackIndex)> pluginNameForTrack;
    std::function<void(int trackIndex)> onPluginLabelClicked;
    std::function<void(int trackIndex)> onEditorButtonClicked;
    std::function<void(int trackIndex)> onChannelLabelClicked;
    std::function<void()> onAddTrackRequested;
    std::function<void(int trackIndex)> onInsertTrackRequested;
    std::function<void(const std::set<int>& trackIndices)> onDuplicateTracksRequested;
    std::function<void(const std::set<int>& trackIndices)> onRemoveTracksRequested;
    std::function<void(const std::set<int>& trackIndices, int gap)> onMoveTracksRequested;
    std::function<void(int trackIndex, juce::String newName)> onTrackRenamed;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    static constexpr int trackRowHeight = 48;
    static constexpr int addButtonRowHeight = 44;
    static constexpr int preferredWidth = 248;

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;

    int getRowIndexAt(int y) const;
    int getGapAt(int y) const;
    bool movesSelectedTracks(int gap) const;
    juce::Rectangle<int> getEditorButtonBounds(int rowIndex) const;
    juce::Rectangle<int> getMuteButtonBounds(int rowIndex) const;
    juce::Rectangle<int> getSoloButtonBounds(int rowIndex) const;
    juce::Rectangle<int> getNameLabelBounds(int rowIndex) const;
    juce::Rectangle<int> getPluginLabelBounds(int rowIndex) const;
    juce::Rectangle<int> getChannelLabelBounds(int rowIndex) const;
    juce::Rectangle<int> getAddButtonBounds() const;
    void updateSize();
    void scrollRowIntoView(int rowIndex);
    void selectRangeTo(int row);
    void toggleMuted(int row);
    void toggleSolo(int row, bool exclusive = false);
    void requestDuplicateTracks(const std::set<int>& indices);
    bool canRemoveTracks(const std::set<int>& indices) const;
    void requestRemoveTracks(const std::set<int>& indices);
    void notifySelectionChanged();
    bool reconcileSelectionWithTracks();

    void beginEditingName(int rowIndex);
    void commitNameEdit();
    void cancelNameEdit();

    MidiSequence* sequence = nullptr;
    std::set<int> selectedTrackIndices = {0};
    int activeTrackIndex = 0;
    int anchorTrackIndex = 0;
    int pressedRow = -1;
    // A press on a track that is part of a multiple selection keeps the selection, so that it can be
    // dragged together, and only selects that track alone when released without dragging.
    bool selectAloneOnRelease = false;
    std::optional<int> dropGap;
    std::unique_ptr<TrackNameEditor> nameEditor;
    int editingRow = -1;
};
