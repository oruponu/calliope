#pragma once

#include "edit/event/NoteRef.h"
#include "engine/recording/RecordingLayer.h"
#include "model/MidiSequence.h"
#include "model/controller/ControllerKind.h"
#include "ui/EventListItems.h"
#include "ui/NoteDragPreview.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <set>
#include <vector>

class EventListComponent : public juce::Component, public MidiSequence::Listener, private juce::ListBoxModel
{
public:
    EventListComponent();
    ~EventListComponent() override;

    void setSequence(MidiSequence* seq);
    void setSelectedTracks(const std::set<int>& selected);
    void refresh();
    void setPlayheadTick(double tick);
    void setSelectedNotes(const std::set<NoteRef>& selected);
    void setSelectedEvents(int trackIndex, const std::set<int>& eventIndices);
    void setRecordingLayer(const RecordingLayer* layer);
    void recordingLayerChanged();
    void setNoteDragPreview(const NoteDragPreview& preview);

    std::function<void(int tick)> onEventSelected;
    std::function<void(const std::set<NoteRef>& noteRefs)> onNoteSelectionFromList;
    // Called after onNoteSelectionFromList when the user picked a control change or pitch bend row, with the
    // selected events of its controller on its track.
    std::function<void(int trackIndex, const ControllerKind& kind, const std::set<int>& eventIndices)>
        onControllerSelectionFromList;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int preferredWidth = 280;
    static constexpr int rowHeight = 25;
    static constexpr int headerHeight = 25;

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;
    void tempoChanged() override;
    void timelineMetadataChanged() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void rebuildList();
    void selectRows(const std::vector<int>& rows);
    void clearSelection();

    static juce::String formatPosition(const BarBeatTick& bbt);
    static juce::String formatEvent(const EventListItem& item);
    static juce::String formatLength(const EventListItem& item);
    static juce::String formatValue(const EventListItem& item);
    static juce::String noteNumberToName(int noteNumber);

    MidiSequence* sequence = nullptr;
    const RecordingLayer* recordingLayer = nullptr;
    std::set<int> selectedTracks;
    std::vector<EventListItem> items;
    NoteDragPreview dragPreview;

    juce::ListBox listBox;
    double playheadTick = 0.0;
    int lastPlayheadRow = -1;
    bool updatingFromPlayhead = false;
    bool updatingFromSelection = false;
};
