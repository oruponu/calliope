#include "ui/EventListComponent.h"
#include "ui/RowSelection.h"
#include "ui/theme/TrackColours.h"
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

EventListComponent::EventListComponent()
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
    updatingFromNoteSelection = true;
    listBox.deselectAllRows();

    int firstRow = -1;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const auto& item = items[static_cast<size_t>(i)];
        if (item.kind == EventListItem::Note && selected.contains(EventListItems::noteRefOf(item)))
        {
            listBox.selectRow(i, true, false);
            if (firstRow < 0)
                firstRow = i;
        }
    }

    if (firstRow >= 0)
    {
        auto* vp = listBox.getViewport();
        int rowTop = firstRow * rowHeight;
        int viewTop = vp->getViewPositionY();
        int viewBottom = viewTop + vp->getViewHeight();

        if (rowTop < viewTop || rowTop + rowHeight > viewBottom)
            vp->setViewPosition(0, rowTop);
    }

    updatingFromNoteSelection = false;
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
    if (updatingFromPlayhead || updatingFromNoteSelection)
        return;

    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(items.size()))
    {
        lastPlayheadRow = lastRowSelected;
        if (onEventSelected)
            onEventSelected(items[static_cast<size_t>(lastRowSelected)].tick);
    }

    if (onNoteSelectionFromList)
    {
        std::set<NoteRef> noteRefs;
        auto selectedRows = listBox.getSelectedRows();
        for (int i = 0; i < selectedRows.size(); ++i)
        {
            int row = selectedRows[i];
            if (row >= 0 && row < static_cast<int>(items.size()))
            {
                const auto& item = items[static_cast<size_t>(row)];
                if (item.kind == EventListItem::Note)
                    noteRefs.insert(EventListItems::noteRefOf(item));
            }
        }
        onNoteSelectionFromList(noteRefs);
    }
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

void EventListComponent::clearSelection()
{
    listBox.setSelectedRows({}, juce::dontSendNotification);
    lastPlayheadRow = -1;
}

void EventListComponent::rebuildList()
{
    const auto selection = listBox.getSelectedRows();
    std::vector<int> selectedRows;
    for (const auto& range : selection.getRanges())
        for (int row = range.getStart(); row < range.getEnd(); ++row)
            selectedRows.push_back(row);

    auto rebuilt = sequence != nullptr ? EventListItems::build(*sequence, selectedTracks, recordingLayer)
                                       : std::vector<EventListItem>{};
    // Rows reorder when notes move, so the selection follows the notes and events rather than the row numbers.
    const auto reselected = RowSelection::fromRows(EventListItems::reselectedRows(items, selectedRows, rebuilt));
    items = std::move(rebuilt);

    // updateContent tells the model when it drops selected rows past the end, and the model would read those stale
    // row numbers against the new rows.
    listBox.setSelectedRows({}, juce::dontSendNotification);
    listBox.updateContent();
    listBox.setSelectedRows(reselected, juce::dontSendNotification);
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
