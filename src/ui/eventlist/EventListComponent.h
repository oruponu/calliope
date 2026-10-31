#pragma once

#include "edit/controller/ControllerPointEditResult.h"
#include "edit/event/NoteRef.h"
#include "engine/recording/RecordingLayer.h"
#include "model/MidiSequence.h"
#include "model/controller/ControllerKind.h"
#include "model/event/MidiNote.h"
#include "ui/drag/NoteDragPreview.h"
#include "ui/eventlist/EditedCell.h"
#include "ui/eventlist/EditorExit.h"
#include "ui/eventlist/EventListItems.h"
#include "ui/eventlist/PositionField.h"
#include "undo/UndoHistory.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <set>
#include <vector>

class EventListComponent : public juce::Component, public MidiSequence::Listener, private juce::ListBoxModel
{
public:
    explicit EventListComponent(UndoHistory& undoHistoryRef);
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
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    void listWasScrolled() override;

    juce::Rectangle<int> positionBounds(int row) const;
    juce::Rectangle<int> eventBounds(int row) const;
    juce::Rectangle<int> lengthBounds(int row) const;
    juce::Rectangle<int> valueBounds(int row) const;
    juce::Rectangle<int> editorBounds(int row) const;
    // The field is where a position edit starts; the other columns' edits have only one.
    void beginEditing(EditedCell cell, PositionField field);
    void showEditor();
    void moveEdited(int tick);
    void setEditedNoteNumber(int noteNumber);
    // A note's or a key pressure's note number, or a control change's controller number.
    void setEditedEventNumber(int number);
    void setEditedLength(int duration);
    // shownValue is as the VAL column shows it.
    void setEditedValue(int shownValue);
    // The event's number, the length or the value, by the edited column.
    void setEditedCellValue(int value);
    // Each edit is one more action in the undo transaction, so the whole edit undoes in one step. The event stays on
    // the editor's row until the edit ends.
    void applyEdit(const juce::String& transactionName, const std::function<bool()>& edit);
    void endEditing(EditorExit exit);
    void cancelEditing();
    void closeEditor();
    // Lists the edited event in order, or rolls the edits back when they are not kept.
    void settleEdits(bool keep);
    void selectAlso(int row);
    bool moveEditedNote(int tick);
    bool modifyEditedNote(const std::function<void(MidiNote&)>& change);
    bool moveEditedEvent(int tick);
    bool setEditedEventValue(int value);
    bool setEditedControllerNumber(int number);
    bool setEditedKeyPressureNote(int number);
    bool replaceEditedEvents(ControllerPointEditResult result);

    void rebuildList();
    void selectRows(const std::vector<int>& rows);
    void clearSelection();

    static juce::String formatPosition(const BarBeatTick& bbt);
    static juce::String formatEvent(const EventListItem& item);
    static juce::String formatLength(const EventListItem& item);
    static juce::String formatValue(const EventListItem& item);

    UndoHistory& undoHistory;
    MidiSequence* sequence = nullptr;
    const RecordingLayer* recordingLayer = nullptr;
    std::set<int> selectedTracks;
    std::vector<EventListItem> items;
    NoteDragPreview dragPreview;

    juce::ListBox listBox;
    std::unique_ptr<juce::Component> editor;
    // The editor's row, which holds the edited event through its edits. Any other rebuild closes the editor.
    int editedRow = -1;
    EditedColumn editedColumn = EditedColumn::Position;
    EventListItem editedItem;
    EventListItem originalItem;
    bool editTransactionOpen = false;
    bool applyingEdit = false;
    double playheadTick = 0.0;
    int lastPlayheadRow = -1;
    bool updatingFromPlayhead = false;
    bool updatingFromSelection = false;
};
