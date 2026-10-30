#include "ui/eventlist/EventListComponent.h"
#include "edit/controller/ChannelEventEdits.h"
#include "model/notation/SysExHex.h"
#include "ui/eventlist/EditableRows.h"
#include "ui/eventlist/MetaEventDescription.h"
#include "ui/eventlist/RowSelection.h"
#include "ui/theme/TrackColours.h"
#include "undo/event/NoteModifyAction.h"
#include "undo/event/ReplaceEventsAction.h"
#include <utility>

namespace
{
constexpr int kPadLeft = 10;
constexpr int kColDot = 16;
constexpr int kColPosition = 84;
constexpr int kColLength = 34;
constexpr int kColValue = 40;
constexpr int kPadRight = 8;
} // namespace

EventListComponent::EventListComponent(UndoHistory& undoHistoryRef) : undoHistory(undoHistoryRef)
{
    using namespace calliope::theme;
    listBox.setModel(this);
    listBox.setRowHeight(rowHeight);
    listBox.setMultipleSelectionEnabled(true);
    listBox.setColour(juce::ListBox::backgroundColourId, surface::bg2);
    addAndMakeVisible(listBox);
}

EventListComponent::~EventListComponent()
{
    if (sequence != nullptr)
        sequence->removeListener(this);
}

void EventListComponent::setSequence(MidiSequence* seq)
{
    if (sequence != nullptr)
        sequence->removeListener(this);
    sequence = seq;
    if (sequence != nullptr)
        sequence->addListener(this);
    clearSelection();
    rebuildList();
}

void EventListComponent::setSelectedTracks(const std::set<int>& selected)
{
    selectedTracks = selected;
    // The piano roll drops its note selection on a track change, so keeping it here would disagree.
    clearSelection();
    rebuildList();
}

void EventListComponent::refresh()
{
    rebuildList();
}

void EventListComponent::setPlayheadTick(double tick)
{
    playheadTick = tick;
    // The drag's highlight and scrolling win until the drag ends; following resumes when the playhead changes rows.
    if (dragPreview.dragging)
        return;

    const int row = EventListItems::playheadRow(items, tick);
    if (row < 0)
        return;

    if (row != lastPlayheadRow)
    {
        lastPlayheadRow = row;
        updatingFromPlayhead = true;
        listBox.selectRow(row, true);
        updatingFromPlayhead = false;
        auto* vp = listBox.getViewport();
        int rowTop = row * rowHeight;
        int viewTop = vp->getViewPositionY();
        int viewBottom = viewTop + vp->getViewHeight();

        if (rowTop < viewTop || rowTop + rowHeight > viewBottom)
            vp->setViewPosition(0, rowTop);
    }
}

void EventListComponent::setSelectedNotes(const std::set<NoteRef>& selected)
{
    std::vector<int> rows;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const auto& item = items[static_cast<size_t>(i)];
        if (item.kind == EventListItem::Note && selected.contains(EventListItems::noteRefOf(item)))
            rows.push_back(i);
    }
    selectRows(rows);
}

void EventListComponent::setSelectedEvents(int trackIndex, const std::set<int>& eventIndices)
{
    selectRows(EventListItems::eventRows(items, trackIndex, eventIndices));
}

void EventListComponent::selectRows(const std::vector<int>& rows)
{
    updatingFromSelection = true;
    listBox.deselectAllRows();
    for (const int row : rows)
        listBox.selectRow(row, true, false);

    if (!rows.empty())
    {
        auto* vp = listBox.getViewport();
        int rowTop = rows.front() * rowHeight;
        int viewTop = vp->getViewPositionY();
        int viewBottom = viewTop + vp->getViewHeight();

        if (rowTop < viewTop || rowTop + rowHeight > viewBottom)
            vp->setViewPosition(0, rowTop);
    }

    updatingFromSelection = false;
}

void EventListComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(surface::bg2);

    auto headerArea = getLocalBounds().removeFromTop(headerHeight);
    g.setColour(surface::surface);
    g.fillRect(headerArea);

    g.setColour(text::t3);
    g.setFont(font::sans(font::sizeXS).boldened());

    const int width = getWidth();

    int contentWidth = width;
    auto& scrollBar = listBox.getVerticalScrollBar();
    if (scrollBar.isVisible())
        contentWidth -= scrollBar.getWidth();

    const int positionX = kPadLeft + kColDot;
    const int eventX = positionX + kColPosition;
    const int valueX = contentWidth - kPadRight - kColValue;
    const int lengthX = valueX - kColLength;
    const int eventWidth = lengthX - eventX;

    g.drawText("POSITION", positionX, headerArea.getY(), kColPosition, headerHeight, juce::Justification::centredLeft);
    g.drawText("EVENT", eventX, headerArea.getY(), eventWidth, headerHeight, juce::Justification::centredLeft);
    g.drawText("LEN", lengthX, headerArea.getY(), kColLength, headerHeight, juce::Justification::centredLeft);
    g.drawText("VAL", valueX, headerArea.getY(), kColValue, headerHeight, juce::Justification::centredLeft);

    g.setColour(border::normal);
    g.drawHorizontalLine(headerHeight - 1, 0.0f, static_cast<float>(width));
}

void EventListComponent::resized()
{
    listBox.setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (positionEditor != nullptr)
        positionEditor->setBounds(positionBounds(editedRow));
}

void EventListComponent::notesChanged(int)
{
    refresh();
}
void EventListComponent::tracksChanged()
{
    refresh();
}
void EventListComponent::tempoChanged()
{
    refresh();
}
void EventListComponent::timelineMetadataChanged()
{
    refresh();
}

int EventListComponent::getNumRows()
{
    return static_cast<int>(items.size());
}

void EventListComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    using namespace calliope::theme;
    if (rowNumber < 0 || rowNumber >= static_cast<int>(items.size()))
        return;

    const auto& item = items[static_cast<size_t>(rowNumber)];

    g.setColour(rowNumber % 2 == 0 ? surface::surface : surface::surface2);
    g.fillRect(0, 0, width, height);
    if (rowIsSelected)
    {
        g.setColour(surface::selection);
        g.fillRect(0, 0, width, height);
        g.setColour(accent::base);
        g.fillRect(0, 0, 2, height);
    }

    constexpr int dotSize = 8;
    const int dotX = kPadLeft;
    const int dotY = (height - dotSize) / 2;
    g.setColour(item.isRecording ? status::danger : TrackColours::getColour(item.trackIndex));
    g.fillRoundedRectangle(static_cast<float>(dotX), static_cast<float>(dotY), static_cast<float>(dotSize),
                           static_cast<float>(dotSize), 2.0f);

    const int positionX = kPadLeft + kColDot;
    const int eventX = positionX + kColPosition;
    const int valueX = width - kPadRight - kColValue;
    const int lengthX = valueX - kColLength;
    const int eventWidth = lengthX - eventX;

    auto bbt = sequence->getTimeline().tickToBarBeatTick(item.tick);

    g.setColour(item.isRecording ? text::t3 : text::t1);
    g.setFont(font::sans(font::sizeSM));

    g.drawText(formatPosition(bbt), positionX, 0, kColPosition, height, juce::Justification::centredLeft);
    g.drawText(formatEvent(item), eventX, 0, eventWidth, height, juce::Justification::centredLeft);
    g.drawText(formatLength(item), lengthX, 0, kColLength, height, juce::Justification::centredRight);
    g.drawText(formatValue(item), valueX, 0, kColValue, height, juce::Justification::centredRight);
}

void EventListComponent::selectedRowsChanged(int lastRowSelected)
{
    if (updatingFromPlayhead || updatingFromSelection)
        return;

    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(items.size()))
    {
        lastPlayheadRow = lastRowSelected;
        if (onEventSelected)
            onEventSelected(items[static_cast<size_t>(lastRowSelected)].tick);
    }

    std::vector<int> selectedRows;
    const auto selection = listBox.getSelectedRows();
    for (int i = 0; i < selection.size(); ++i)
        selectedRows.push_back(selection[i]);

    if (onNoteSelectionFromList)
    {
        std::set<NoteRef> noteRefs;
        for (const int row : selectedRows)
        {
            if (row >= 0 && row < static_cast<int>(items.size()))
            {
                const auto& item = items[static_cast<size_t>(row)];
                if (item.kind == EventListItem::Note)
                    noteRefs.insert(EventListItems::noteRefOf(item));
            }
        }
        onNoteSelectionFromList(noteRefs);
    }

    if (onControllerSelectionFromList && lastRowSelected >= 0 && lastRowSelected < static_cast<int>(items.size()))
    {
        const auto& clicked = items[static_cast<size_t>(lastRowSelected)];
        if (const auto kind = EventListItems::controllerOf(clicked))
            onControllerSelectionFromList(clicked.trackIndex, *kind,
                                          EventListItems::sameControllerEvents(items, selectedRows, lastRowSelected));
    }
}

void EventListComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent& e)
{
    const auto bounds = positionBounds(row);
    if (e.x >= bounds.getX() && e.x < bounds.getRight())
        beginEditing(row, PositionEditor::fieldAt(e.x - bounds.getX()));
}

void EventListComponent::listWasScrolled()
{
    if (positionEditor != nullptr)
        positionEditor->setBounds(positionBounds(editedRow));
}

juce::Rectangle<int> EventListComponent::positionBounds(int row) const
{
    return listBox.getRowPosition(row, true).withX(kPadLeft + kColDot).withWidth(kColPosition);
}

void EventListComponent::beginEditing(int row, PositionField field)
{
    closeEditor();
    if (sequence == nullptr || row < 0 || row >= static_cast<int>(items.size()) ||
        !EditableRows::isEditable(items[static_cast<size_t>(row)]))
        return;

    editedRow = row;
    editedItem = items[static_cast<size_t>(row)];
    originalItem = editedItem;
    moveTransactionOpen = false;
    listBox.scrollToEnsureRowIsOnscreen(row);

    const auto& timeline = sequence->getTimeline();
    positionEditor = std::make_unique<PositionEditor>(timeline, timeline.tickToBarBeatTick(editedItem.tick));
    positionEditor->onWheelStep = [this](int tick) { moveEdited(tick); };
    // Ending the edit destroys the editor along with these lambdas, so they copy this before ending it.
    positionEditor->onCommit = [this](int tick, PositionEditor::Exit exit)
    {
        auto* list = this;
        list->endEditing(tick, exit);
    };
    positionEditor->onCancel = [this]()
    {
        auto* list = this;
        list->cancelEditing();
        list->grabKeyboardFocus();
    };
    positionEditor->setBounds(positionBounds(row));
    listBox.addAndMakeVisible(*positionEditor);
    positionEditor->focusField(field);
}

void EventListComponent::moveEdited(int tick)
{
    // Moving an event onto its own tick would still put it after the others there.
    if (tick == editedItem.tick)
        return;

    const bool wasSelected = listBox.isRowSelected(editedRow);
    if (!moveTransactionOpen)
        undoHistory.beginNewTransaction(editedItem.kind == EventListItem::Note ? "Move Note" : "Move Event");
    {
        // The move's own rebuild leaves the editor open and the event on the editor's row.
        const juce::ScopedValueSetter<bool> moving(movingEdited, true);
        const bool moved = editedItem.kind == EventListItem::Note ? moveEditedNote(tick) : moveEditedEvent(tick);
        if (!moved)
            return;
    }
    moveTransactionOpen = true;

    // The editor's row is past the end when the move hides points, and the rebuild then puts the event last.
    const auto row = EventListItems::rowOf(items, editedItem);
    if (!row)
        return;
    editedRow = *row;
    editedItem = items[static_cast<size_t>(*row)];
    if (wasSelected)
        selectAlso(*row);
    if (positionEditor != nullptr)
        positionEditor->setBounds(positionBounds(*row));
}

void EventListComponent::endEditing(int tick, PositionEditor::Exit exit)
{
    moveEdited(tick);
    closeEditor();
    settleMoves(true);
    if (exit == PositionEditor::Exit::ReturnKey)
        grabKeyboardFocus();
}

void EventListComponent::cancelEditing()
{
    closeEditor();
    settleMoves(false);
}

void EventListComponent::closeEditor()
{
    if (positionEditor == nullptr)
        return;
    auto editor = std::move(positionEditor);
    editor.reset();
}

void EventListComponent::settleMoves(bool keep)
{
    if (!moveTransactionOpen)
        return;

    const bool wasSelected = listBox.isRowSelected(editedRow);
    // Moves that came back to where the event was leave no undo step.
    if (keep && editedItem.tick != originalItem.tick)
    {
        rebuildList();
    }
    else
    {
        undoHistory.cancelCurrentTransaction();
        editedItem = originalItem;
    }
    moveTransactionOpen = false;

    const auto row = EventListItems::rowOf(items, editedItem);
    if (!row)
        return;
    editedRow = *row;
    if (wasSelected)
        selectAlso(*row);
    listBox.scrollToEnsureRowIsOnscreen(*row);
}

void EventListComponent::selectAlso(int row)
{
    // The rebuild finds a selected event by its values, which the move changed.
    updatingFromSelection = true;
    listBox.selectRow(row, true, false);
    updatingFromSelection = false;
}

bool EventListComponent::moveEditedNote(int tick)
{
    const auto& item = editedItem;
    if (item.trackIndex >= sequence->getNumTracks() || item.sourceIndex < 0 ||
        item.sourceIndex >= sequence->getTrack(item.trackIndex).getNumNotes())
        return false;

    const auto before = sequence->getTrack(item.trackIndex).getNote(item.sourceIndex);
    auto after = before;
    after.startTick = tick;
    return undoHistory.perform(new NoteModifyAction(sequence, item.trackIndex, item.sourceIndex, before, after));
}

bool EventListComponent::moveEditedEvent(int tick)
{
    if (editedItem.trackIndex >= sequence->getNumTracks())
        return false;

    const auto& track = sequence->getTrack(editedItem.trackIndex);
    auto result = ChannelEventEdits::moveTo(track.getEvents(), editedItem.sourceIndex, tick);
    if (result.selected.empty())
        return false;

    // Set first: the move's rebuild finds the event by its new index to keep it on its row.
    const int previousIndex = std::exchange(editedItem.sourceIndex, *result.selected.begin());
    if (undoHistory.perform(
            new ReplaceEventsAction(sequence, track.getId(), track.getEvents(), std::move(result.events))))
        return true;
    editedItem.sourceIndex = previousIndex;
    return false;
}

void EventListComponent::setRecordingLayer(const RecordingLayer* layer)
{
    recordingLayer = layer;
    rebuildList();
}

void EventListComponent::recordingLayerChanged()
{
    rebuildList();
}

void EventListComponent::setNoteDragPreview(const NoteDragPreview& preview)
{
    dragPreview = preview;
    rebuildList();
}

void EventListComponent::clearSelection()
{
    listBox.setSelectedRows({}, juce::dontSendNotification);
    lastPlayheadRow = -1;
}

void EventListComponent::rebuildList()
{
    if (!movingEdited)
    {
        // Another change may have moved the event; the moves already made stay as an undo step.
        closeEditor();
        moveTransactionOpen = false;
    }

    const auto selection = listBox.getSelectedRows();
    std::vector<int> selectedRows;
    for (const auto& range : selection.getRanges())
        for (int row = range.getStart(); row < range.getEnd(); ++row)
            selectedRows.push_back(row);

    auto rebuilt = sequence != nullptr ? EventListItems::build(*sequence, selectedTracks, recordingLayer, dragPreview)
                                       : std::vector<EventListItem>{};
    // The edited event stays on the editor's row until the edit ends, so the wheel stays over the editor.
    if (movingEdited)
        rebuilt = EventListItems::withRowAt(std::move(rebuilt), editedItem, editedRow);
    // Rows reorder when notes move, so the selection follows the notes and events rather than the row numbers. Dragged
    // points change their values, so they are found by their indices in the drag instead.
    const auto reselected = RowSelection::fromRows(dragPreview.draggedEvents.has_value()
                                                       ? EventListItems::draggedRows(rebuilt, dragPreview)
                                                       : EventListItems::reselectedRows(items, selectedRows, rebuilt));
    items = std::move(rebuilt);

    // updateContent tells the model when it drops selected rows past the end, and the model would read those stale
    // row numbers against the new rows.
    listBox.setSelectedRows({}, juce::dontSendNotification);
    listBox.updateContent();
    listBox.setSelectedRows(reselected, juce::dontSendNotification);
    if (const auto row = EventListItems::focusRow(items, dragPreview))
        listBox.scrollToEnsureRowIsOnscreen(*row);
    // A stale row number would make the next playhead update misjudge whether the playhead changed rows.
    if (lastPlayheadRow >= 0)
        lastPlayheadRow = EventListItems::playheadRow(items, playheadTick);
    listBox.repaint();
    repaint();
}

juce::String EventListComponent::formatPosition(const BarBeatTick& bbt)
{
    return juce::String(bbt.bar).paddedLeft('0', 3) + "." + juce::String(bbt.beat).paddedLeft('0', 2) + "." +
           juce::String(bbt.tick).paddedLeft('0', 4);
}

juce::String EventListComponent::formatEvent(const EventListItem& item)
{
    switch (item.kind)
    {
    case EventListItem::Meta:
        return MetaEventDescription::describe(item.data1, item.metaData);
    case EventListItem::SysEx:
        return "SysEx F0 " + juce::String(SysExHex::format(item.sysExData)) + " F7";
    case EventListItem::Note:
        return "Note " + noteNumberToName(item.noteNumber);
    case EventListItem::ControlChange:
        return "CC " + juce::String(item.data1);
    case EventListItem::ProgramChange:
        return "PC " + juce::String(item.data1);
    case EventListItem::PitchBend:
        return "Pitch Bend";
    case EventListItem::ChannelPressure:
        return "Ch Press";
    case EventListItem::KeyPressure:
        return "Key Press";
    }
    return {};
}

juce::String EventListComponent::formatLength(const EventListItem& item)
{
    if (item.kind == EventListItem::Note)
        return juce::String(item.duration);
    return "--";
}

juce::String EventListComponent::formatValue(const EventListItem& item)
{
    switch (item.kind)
    {
    case EventListItem::Meta:
        return "--";
    case EventListItem::SysEx:
        return "--";
    case EventListItem::Note:
        return juce::String(item.velocity);
    case EventListItem::ControlChange:
        return juce::String(item.data2);
    case EventListItem::ProgramChange:
        return juce::String(item.data1);
    case EventListItem::PitchBend:
        return juce::String(item.data1 - 8192);
    case EventListItem::ChannelPressure:
        return juce::String(item.data1);
    case EventListItem::KeyPressure:
        return juce::String(item.data2);
    }
    return {};
}

juce::String EventListComponent::noteNumberToName(int noteNumber)
{
    static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (noteNumber / 12) - 1;
    int note = noteNumber % 12;
    return juce::String(names[note]) + juce::String(octave);
}
