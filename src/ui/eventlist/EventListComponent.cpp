#include "ui/eventlist/EventListComponent.h"
#include "edit/controller/ChannelEventEdits.h"
#include "model/notation/NoteName.h"
#include "model/notation/SysExHex.h"
#include "ui/eventlist/EditableRows.h"
#include "ui/eventlist/LengthText.h"
#include "ui/eventlist/MetaEventDescription.h"
#include "ui/eventlist/PositionEditor.h"
#include "ui/eventlist/RowSelection.h"
#include "ui/eventlist/ValueEditor.h"
#include "ui/eventlist/ValueFormat.h"
#include "ui/eventlist/ValueText.h"
#include "ui/theme/TrackColours.h"
#include "undo/event/NoteModifyAction.h"
#include "undo/event/ReplaceEventsAction.h"
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace
{
constexpr int kPadLeft = 10;
constexpr int kColDot = 16;
constexpr int kColPosition = 84;
constexpr int kColNote = 36;
constexpr int kColLength = 34;
constexpr int kColValue = 40;
constexpr int kPadRight = 8;
constexpr int kPitchBendCentre = 8192;

ValueFormat noteNumberFormat()
{
    return {.format = NoteName::format,
            .parse = NoteName::parse,
            .minimum = 0,
            .maximum = 127,
            .maxLength = 4,
            .allowedCharacters = "0123456789ABCDEFGabcdefg#-",
            .justification = juce::Justification::centredLeft};
}

ValueFormat lengthFormat()
{
    return {.format = [](int ticks) { return std::to_string(ticks); },
            .parse = LengthText::parse,
            .minimum = 1,
            .maximum = LengthText::kMaxLength,
            .maxLength = 7,
            .allowedCharacters = "0123456789",
            .justification = juce::Justification::centredRight};
}

ValueFormat numberFormat(int minimum, int maximum)
{
    return {.format = [](int value) { return std::to_string(value); },
            .parse = [minimum, maximum](std::string_view text) { return ValueText::parse(text, minimum, maximum); },
            .minimum = minimum,
            .maximum = maximum,
            .maxLength = static_cast<int>(std::max(std::to_string(minimum).size(), std::to_string(maximum).size())),
            .allowedCharacters = minimum < 0 ? "-0123456789" : "0123456789",
            .justification = juce::Justification::centredRight};
}

ValueFormat controllerNumberFormat()
{
    auto format = numberFormat(0, 127);
    format.justification = juce::Justification::centredLeft;
    return format;
}

// The number the EVENT column shows: a note's or a key pressure's note number, or a control change's controller
// number.
int eventNumberOf(const EventListItem& item)
{
    return item.kind == EventListItem::Note ? item.noteNumber : item.data1;
}

ValueFormat eventFormatOf(const EventListItem& item)
{
    return item.kind == EventListItem::ControlChange ? controllerNumberFormat() : noteNumberFormat();
}

// The value the VAL column shows: a note's velocity, a channel event's value with a pitch bend's centred on zero.
int shownValueOf(const EventListItem& item)
{
    switch (item.kind)
    {
    case EventListItem::Note:
        return item.velocity;
    case EventListItem::ControlChange:
    case EventListItem::KeyPressure:
        return item.data2;
    case EventListItem::PitchBend:
        return item.data1 - kPitchBendCentre;
    case EventListItem::ProgramChange:
    case EventListItem::ChannelPressure:
    case EventListItem::Meta:
    case EventListItem::SysEx:
        break;
    }
    return item.data1;
}

ValueFormat valueFormatOf(const EventListItem& item)
{
    if (item.kind == EventListItem::Note)
        return numberFormat(1, 127);
    if (item.kind == EventListItem::PitchBend)
        return numberFormat(-kPitchBendCentre, kPitchBendCentre - 1);
    return numberFormat(0, 127);
}

// Whether the edits left the event as it was, for every value the event list edits.
bool sameEdited(const EventListItem& a, const EventListItem& b)
{
    return a.tick == b.tick && a.noteNumber == b.noteNumber && a.duration == b.duration && a.velocity == b.velocity &&
           a.data1 == b.data1 && a.data2 == b.data2;
}
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
    if (editor != nullptr)
        editor->setBounds(editorBounds(editedRow));
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
    const auto position = positionBounds(row);
    if (e.x >= position.getX() && e.x < position.getRight())
    {
        beginEditing({row, EditedColumn::Position}, PositionEditor::fieldAt(e.x - position.getX()));
        return;
    }
    const auto event = eventBounds(row);
    if (e.x >= event.getX() && e.x < event.getRight())
    {
        beginEditing({row, EditedColumn::Event}, PositionField::Bar);
        return;
    }
    const auto length = lengthBounds(row);
    if (e.x >= length.getX() && e.x < length.getRight())
    {
        beginEditing({row, EditedColumn::Length}, PositionField::Bar);
        return;
    }
    const auto value = valueBounds(row);
    if (e.x >= value.getX() && e.x < value.getRight())
        beginEditing({row, EditedColumn::Value}, PositionField::Bar);
}

void EventListComponent::listWasScrolled()
{
    if (editor != nullptr)
        editor->setBounds(editorBounds(editedRow));
}

juce::Rectangle<int> EventListComponent::positionBounds(int row) const
{
    return listBox.getRowPosition(row, true).withX(kPadLeft + kColDot).withWidth(kColPosition);
}

juce::Rectangle<int> EventListComponent::eventBounds(int row) const
{
    using namespace calliope::theme;
    // Over the note name or the controller number, after the label the row draws before it.
    const auto kind =
        row >= 0 && row < static_cast<int>(items.size()) ? items[static_cast<size_t>(row)].kind : EventListItem::Note;
    const auto* label = kind == EventListItem::ControlChange ? "CC "
                        : kind == EventListItem::KeyPressure ? "Key Press "
                                                             : "Note ";
    const int labelWidth = juce::GlyphArrangement::getStringWidthInt(font::sans(font::sizeSM), label);
    return listBox.getRowPosition(row, true).withX(kPadLeft + kColDot + kColPosition + labelWidth).withWidth(kColNote);
}

juce::Rectangle<int> EventListComponent::lengthBounds(int row) const
{
    const auto rowBounds = listBox.getRowPosition(row, true);
    return rowBounds.withX(rowBounds.getWidth() - kPadRight - kColValue - kColLength).withWidth(kColLength);
}

juce::Rectangle<int> EventListComponent::valueBounds(int row) const
{
    const auto rowBounds = listBox.getRowPosition(row, true);
    return rowBounds.withX(rowBounds.getWidth() - kPadRight - kColValue).withWidth(kColValue);
}

juce::Rectangle<int> EventListComponent::editorBounds(int row) const
{
    switch (editedColumn)
    {
    case EditedColumn::Position:
        return positionBounds(row);
    case EditedColumn::Event:
        return eventBounds(row);
    case EditedColumn::Length:
        return lengthBounds(row);
    case EditedColumn::Value:
        return valueBounds(row);
    }
    return {};
}

void EventListComponent::beginEditing(EditedCell cell, PositionField field)
{
    closeEditor();
    if (sequence == nullptr || cell.row < 0 || cell.row >= static_cast<int>(items.size()))
        return;
    const auto& item = items[static_cast<size_t>(cell.row)];
    const bool editable = cell.column == EditedColumn::Position ? EditableRows::isEditable(item)
                          : cell.column == EditedColumn::Event  ? EditableRows::isEventEditable(item)
                          : cell.column == EditedColumn::Length ? EditableRows::isLengthEditable(item)
                                                                : EditableRows::isValueEditable(item);
    if (!editable)
        return;

    editedRow = cell.row;
    editedColumn = cell.column;
    editedItem = item;
    originalItem = editedItem;
    editTransactionOpen = false;
    listBox.scrollToEnsureRowIsOnscreen(cell.row);

    // Ending the edit destroys the editor along with these lambdas, so they copy this before ending it.
    const auto cancel = [this]()
    {
        auto* list = this;
        list->cancelEditing();
        list->grabKeyboardFocus();
    };
    if (cell.column == EditedColumn::Position)
    {
        const auto& timeline = sequence->getTimeline();
        auto positionEditor = std::make_unique<PositionEditor>(timeline, timeline.tickToBarBeatTick(editedItem.tick));
        positionEditor->onWheelStep = [this](int tick) { moveEdited(tick); };
        positionEditor->onCommit = [this](int tick, EditorExit exit)
        {
            auto* list = this;
            list->moveEdited(tick);
            list->endEditing(exit);
        };
        positionEditor->onCancel = cancel;
        auto& shown = *positionEditor;
        editor = std::move(positionEditor);
        showEditor();
        shown.focusField(field);
    }
    else
    {
        auto valueEditor = cell.column == EditedColumn::Event
                               ? std::make_unique<ValueEditor>(eventNumberOf(editedItem), eventFormatOf(editedItem))
                           : cell.column == EditedColumn::Length
                               ? std::make_unique<ValueEditor>(editedItem.duration, lengthFormat())
                               : std::make_unique<ValueEditor>(shownValueOf(editedItem), valueFormatOf(editedItem));
        if (cell.column != EditedColumn::Event || editedItem.kind != EventListItem::KeyPressure)
            valueEditor->onWheelStep = [this](int value) { setEditedCellValue(value); };
        valueEditor->onCommit = [this](int value, EditorExit exit)
        {
            auto* list = this;
            list->setEditedCellValue(value);
            list->endEditing(exit);
        };
        valueEditor->onCancel = cancel;
        auto& shown = *valueEditor;
        editor = std::move(valueEditor);
        showEditor();
        shown.focusField();
    }
}

void EventListComponent::showEditor()
{
    editor->setBounds(editorBounds(editedRow));
    listBox.addAndMakeVisible(*editor);
}

void EventListComponent::moveEdited(int tick)
{
    // Moving an event onto its own tick would still put it after the others there.
    if (tick == editedItem.tick)
        return;
    const bool isNote = editedItem.kind == EventListItem::Note;
    applyEdit(isNote ? "Move Note" : "Move Event",
              [this, isNote, tick]() { return isNote ? moveEditedNote(tick) : moveEditedEvent(tick); });
}

void EventListComponent::setEditedNoteNumber(int noteNumber)
{
    if (noteNumber == editedItem.noteNumber)
        return;
    applyEdit("Move Note", [this, noteNumber]()
              { return modifyEditedNote([noteNumber](MidiNote& note) { note.noteNumber = noteNumber; }); });
}

void EventListComponent::setEditedEventNumber(int number)
{
    if (editedItem.kind == EventListItem::Note)
    {
        setEditedNoteNumber(number);
        return;
    }
    if (number == editedItem.data1)
        return;
    const bool isControlChange = editedItem.kind == EventListItem::ControlChange;
    applyEdit(isControlChange ? "Edit Controller Number" : "Edit Key Pressure Note", [this, isControlChange, number]()
              { return isControlChange ? setEditedControllerNumber(number) : setEditedKeyPressureNote(number); });
}

void EventListComponent::setEditedLength(int duration)
{
    if (duration == editedItem.duration)
        return;
    applyEdit("Resize Note", [this, duration]()
              { return modifyEditedNote([duration](MidiNote& note) { note.duration = duration; }); });
}

void EventListComponent::setEditedValue(int shownValue)
{
    if (shownValue == shownValueOf(editedItem))
        return;
    if (editedItem.kind == EventListItem::Note)
    {
        applyEdit("Edit Velocity", [this, shownValue]()
                  { return modifyEditedNote([shownValue](MidiNote& note) { note.velocity = shownValue; }); });
        return;
    }
    const int value = editedItem.kind == EventListItem::PitchBend ? shownValue + kPitchBendCentre : shownValue;
    applyEdit("Edit Event Value", [this, value]() { return setEditedEventValue(value); });
}

void EventListComponent::setEditedCellValue(int value)
{
    switch (editedColumn)
    {
    case EditedColumn::Event:
        setEditedEventNumber(value);
        break;
    case EditedColumn::Length:
        setEditedLength(value);
        break;
    case EditedColumn::Value:
        setEditedValue(value);
        break;
    case EditedColumn::Position:
        break;
    }
}

void EventListComponent::applyEdit(const juce::String& transactionName, const std::function<bool()>& edit)
{
    const bool wasSelected = listBox.isRowSelected(editedRow);
    if (!editTransactionOpen)
        undoHistory.beginNewTransaction(transactionName);
    {
        // The edit's own rebuild leaves the editor open and the event on the editor's row.
        const juce::ScopedValueSetter<bool> applying(applyingEdit, true);
        if (!edit())
            return;
    }
    editTransactionOpen = true;

    // The editor's row is past the end when a move hides points, and the rebuild then puts the event last.
    const auto row = EventListItems::rowOf(items, editedItem);
    if (!row)
        return;
    editedRow = *row;
    editedItem = items[static_cast<size_t>(*row)];
    if (wasSelected)
        selectAlso(*row);
    if (editor != nullptr)
        editor->setBounds(editorBounds(*row));
}

void EventListComponent::endEditing(EditorExit exit)
{
    closeEditor();
    settleEdits(true);

    if (exit == EditorExit::FocusLost)
        return;
    if (exit == EditorExit::ReturnKey)
    {
        grabKeyboardFocus();
        return;
    }

    const bool backwards = exit == EditorExit::ShiftTab;
    if (const auto next = EditableRows::next(items, {editedRow, editedColumn}, backwards))
        beginEditing(*next, backwards ? PositionField::Tick : PositionField::Bar);
    else
        grabKeyboardFocus();
}

void EventListComponent::cancelEditing()
{
    closeEditor();
    settleEdits(false);
}

void EventListComponent::closeEditor()
{
    if (editor == nullptr)
        return;
    auto closed = std::move(editor);
    closed.reset();
}

void EventListComponent::settleEdits(bool keep)
{
    if (!editTransactionOpen)
        return;

    const bool wasSelected = listBox.isRowSelected(editedRow);
    // Edits that left the event as it was leave no undo step.
    if (keep && !sameEdited(editedItem, originalItem))
    {
        rebuildList();
    }
    else
    {
        undoHistory.cancelCurrentTransaction();
        editedItem = originalItem;
    }
    editTransactionOpen = false;

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
    // Reselects the edited row, which the rebuild loses because it finds a selected event by its values and the edit
    // changed them.
    updatingFromSelection = true;
    listBox.selectRow(row, true, false);
    updatingFromSelection = false;
}

bool EventListComponent::moveEditedNote(int tick)
{
    return modifyEditedNote([tick](MidiNote& note) { note.startTick = tick; });
}

bool EventListComponent::modifyEditedNote(const std::function<void(MidiNote&)>& change)
{
    const auto& item = editedItem;
    if (item.trackIndex >= sequence->getNumTracks() || item.sourceIndex < 0 ||
        item.sourceIndex >= sequence->getTrack(item.trackIndex).getNumNotes())
        return false;

    const auto before = sequence->getTrack(item.trackIndex).getNote(item.sourceIndex);
    auto after = before;
    change(after);
    return undoHistory.perform(new NoteModifyAction(sequence, item.trackIndex, item.sourceIndex, before, after));
}

bool EventListComponent::moveEditedEvent(int tick)
{
    if (editedItem.trackIndex >= sequence->getNumTracks())
        return false;
    const auto& events = sequence->getTrack(editedItem.trackIndex).getEvents();
    return replaceEditedEvents(ChannelEventEdits::moveTo(events, editedItem.sourceIndex, tick));
}

bool EventListComponent::setEditedEventValue(int value)
{
    if (editedItem.trackIndex >= sequence->getNumTracks())
        return false;
    const auto& events = sequence->getTrack(editedItem.trackIndex).getEvents();
    return replaceEditedEvents(ChannelEventEdits::setValue(events, editedItem.sourceIndex, value));
}

bool EventListComponent::setEditedControllerNumber(int number)
{
    if (editedItem.trackIndex >= sequence->getNumTracks())
        return false;
    const auto& events = sequence->getTrack(editedItem.trackIndex).getEvents();
    return replaceEditedEvents(ChannelEventEdits::setControllerNumber(events, editedItem.sourceIndex, number));
}

bool EventListComponent::setEditedKeyPressureNote(int number)
{
    if (editedItem.trackIndex >= sequence->getNumTracks())
        return false;
    const auto& events = sequence->getTrack(editedItem.trackIndex).getEvents();
    return replaceEditedEvents(ChannelEventEdits::setNoteNumber(events, editedItem.sourceIndex, number));
}

bool EventListComponent::replaceEditedEvents(ControllerPointEditResult result)
{
    if (result.selected.empty())
        return false;

    const auto& track = sequence->getTrack(editedItem.trackIndex);
    // Set first: the edit's rebuild finds the event by its new index to keep it on its row.
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
    if (!applyingEdit)
    {
        // Another change may have moved the event; the moves already made stay as an undo step.
        closeEditor();
        editTransactionOpen = false;
    }

    const auto selection = listBox.getSelectedRows();
    std::vector<int> selectedRows;
    for (const auto& range : selection.getRanges())
        for (int row = range.getStart(); row < range.getEnd(); ++row)
            selectedRows.push_back(row);

    auto rebuilt = sequence != nullptr ? EventListItems::build(*sequence, selectedTracks, recordingLayer, dragPreview)
                                       : std::vector<EventListItem>{};
    // The edited event stays on the editor's row until the edit ends, so the wheel stays over the editor.
    if (applyingEdit)
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
        return "Note " + juce::String(NoteName::format(item.noteNumber));
    case EventListItem::ControlChange:
        return "CC " + juce::String(item.data1);
    case EventListItem::ProgramChange:
        return "PC " + juce::String(item.data1);
    case EventListItem::PitchBend:
        return "Pitch Bend";
    case EventListItem::ChannelPressure:
        return "Ch Press";
    case EventListItem::KeyPressure:
        return "Key Press " + juce::String(NoteName::format(item.data1));
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
    if (item.kind == EventListItem::Meta || item.kind == EventListItem::SysEx)
        return "--";
    return juce::String(shownValueOf(item));
}
