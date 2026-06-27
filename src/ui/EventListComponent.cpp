#include "EventListComponent.h"
#include "TrackColours.h"
#include <algorithm>

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

void EventListComponent::setSequence(MidiSequence* seq)
{
    sequence = seq;
    rebuildList();
}

void EventListComponent::setSelectedTracks(const std::set<int>& selected)
{
    selectedTracks = selected;
    rebuildList();
}

void EventListComponent::refresh()
{
    rebuildList();
}

void EventListComponent::rebuildList()
{
    items.clear();

    if (!sequence)
    {
        listBox.updateContent();
        listBox.repaint();
        repaint();
        return;
    }

    for (int trackIdx : selectedTracks)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(trackIdx);

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            EventListItem item;
            item.kind = EventListItem::Note;
            item.tick = note.startTick;
            item.trackIndex = trackIdx;
            item.sourceIndex = i;
            item.noteNumber = note.noteNumber;
            item.duration = note.duration;
            item.velocity = note.velocity;
            items.push_back(item);
        }

        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            const auto& ev = track.getEvent(i);
            EventListItem item;
            item.tick = ev.tick;
            item.trackIndex = trackIdx;
            item.sourceIndex = i;
            item.data1 = ev.data1;
            item.data2 = ev.data2;

            switch (ev.type)
            {
            case MidiEvent::Type::ControlChange:
                item.kind = EventListItem::ControlChange;
                break;
            case MidiEvent::Type::ProgramChange:
                item.kind = EventListItem::ProgramChange;
                break;
            case MidiEvent::Type::PitchBend:
                item.kind = EventListItem::PitchBend;
                break;
            case MidiEvent::Type::ChannelPressure:
                item.kind = EventListItem::ChannelPressure;
                break;
            case MidiEvent::Type::KeyPressure:
                item.kind = EventListItem::KeyPressure;
                break;
            }

            items.push_back(item);
        }
    }

    std::sort(items.begin(), items.end(),
              [](const EventListItem& a, const EventListItem& b)
              {
                  if (a.tick != b.tick)
                      return a.tick < b.tick;
                  if (a.trackIndex != b.trackIndex)
                      return a.trackIndex < b.trackIndex;
                  return a.kind < b.kind;
              });

    listBox.updateContent();
    listBox.repaint();
    repaint();
}

void EventListComponent::setPlayheadTick(double tick)
{
    if (items.empty())
        return;

    auto it = std::upper_bound(items.begin(), items.end(), static_cast<int>(tick),
                               [](int t, const EventListItem& item) { return t < item.tick; });

    int row;
    if (it == items.begin())
    {
        row = 0;
    }
    else
    {
        --it;
        if (tick == static_cast<double>(it->tick))
        {
            auto first = std::lower_bound(items.begin(), it, it->tick,
                                          [](const EventListItem& item, int t) { return item.tick < t; });
            row = static_cast<int>(std::distance(items.begin(), first));
        }
        else
        {
            row = static_cast<int>(std::distance(items.begin(), it));
        }
    }

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

void EventListComponent::setSelectedNotes(const std::set<std::pair<int, int>>& selected)
{
    updatingFromNoteSelection = true;
    listBox.deselectAllRows();

    int firstRow = -1;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const auto& item = items[static_cast<size_t>(i)];
        if (item.kind == EventListItem::Note && selected.contains({item.trackIndex, item.sourceIndex}))
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
        std::set<std::pair<int, int>> noteRefs;
        auto selectedRows = listBox.getSelectedRows();
        for (int i = 0; i < selectedRows.size(); ++i)
        {
            int row = selectedRows[i];
            if (row >= 0 && row < static_cast<int>(items.size()))
            {
                const auto& item = items[static_cast<size_t>(row)];
                if (item.kind == EventListItem::Note)
                    noteRefs.insert({item.trackIndex, item.sourceIndex});
            }
        }
        onNoteSelectionFromList(noteRefs);
    }
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
        g.setColour(accent::soft);
        g.fillRect(0, 0, width, height);
    }

    constexpr int dotSize = 8;
    const int dotX = kPadLeft;
    const int dotY = (height - dotSize) / 2;
    g.setColour(TrackColours::getColour(item.trackIndex));
    g.fillRoundedRectangle(static_cast<float>(dotX), static_cast<float>(dotY), static_cast<float>(dotSize),
                           static_cast<float>(dotSize), 2.0f);

    const int positionX = kPadLeft + kColDot;
    const int eventX = positionX + kColPosition;
    const int valueX = width - kPadRight - kColValue;
    const int lengthX = valueX - kColLength;
    const int eventWidth = lengthX - eventX;

    auto bbt = sequence->tickToBarBeatTick(item.tick);

    g.setColour(text::t1);
    g.setFont(font::sans(font::sizeSM));

    g.drawText(formatPosition(bbt), positionX, 0, kColPosition, height, juce::Justification::centredLeft);
    g.drawText(formatEvent(item), eventX, 0, eventWidth, height, juce::Justification::centredLeft);
    g.drawText(formatLength(item), lengthX, 0, kColLength, height, juce::Justification::centredRight);
    g.drawText(formatValue(item), valueX, 0, kColValue, height, juce::Justification::centredRight);
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
