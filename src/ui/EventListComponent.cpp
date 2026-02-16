#include "EventListComponent.h"
#include "TrackColours.h"
#include <algorithm>

EventListComponent::EventListComponent()
{
    listBox.setModel(this);
    listBox.setRowHeight(rowHeight);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(30, 30, 38));
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
            item.channel = note.channel;
            items.push_back(item);
        }

        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            const auto& ev = track.getEvent(i);
            EventListItem item;
            item.tick = ev.tick;
            item.trackIndex = trackIdx;
            item.sourceIndex = i;
            item.channel = ev.channel;
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
        listBox.selectRow(row);
        updatingFromPlayhead = false;
        int visibleRows = listBox.getHeight() / rowHeight;
        int lookAhead = juce::jmin(row + visibleRows / 2, getNumRows() - 1);
        listBox.scrollToEnsureRowIsOnscreen(lookAhead);
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
        if (item.kind == EventListItem::Note && selected.count({item.trackIndex, item.sourceIndex}) > 0)
        {
            listBox.selectRow(i, true, false);
            if (firstRow < 0)
                firstRow = i;
        }
    }

    if (firstRow >= 0)
    {
        int visibleRows = listBox.getHeight() / rowHeight;
        int lookAhead = juce::jmin(firstRow + visibleRows / 2, getNumRows() - 1);
        listBox.scrollToEnsureRowIsOnscreen(lookAhead);
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
}

void EventListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(30, 30, 38));

    auto headerArea = getLocalBounds().removeFromTop(headerHeight);
    g.setColour(juce::Colour(40, 40, 52));
    g.fillRect(headerArea);

    g.setColour(juce::Colour(140, 140, 160));
    g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());

    int x = 6;
    g.drawText("Position", x, headerArea.getY(), colPosition, headerHeight, juce::Justification::centredLeft);
    x += colPosition;
    g.drawText("Event", x, headerArea.getY(), colEvent, headerHeight, juce::Justification::centredLeft);
    x += colEvent;
    g.drawText("Length", x, headerArea.getY(), colLength, headerHeight, juce::Justification::centredLeft);
    x += colLength;
    g.drawText("Value", x, headerArea.getY(), colValue, headerHeight, juce::Justification::centredLeft);

    g.setColour(juce::Colour(60, 60, 70));
    g.drawHorizontalLine(headerHeight - 1, 0.0f, static_cast<float>(getWidth()));
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
    if (rowNumber < 0 || rowNumber >= static_cast<int>(items.size()))
        return;

    const auto& item = items[static_cast<size_t>(rowNumber)];

    if (rowIsSelected)
        g.setColour(juce::Colour(55, 65, 90));
    else if (rowNumber % 2 == 0)
        g.setColour(juce::Colour(32, 32, 40));
    else
        g.setColour(juce::Colour(36, 36, 44));
    g.fillRect(0, 0, width, height);

    g.setColour(TrackColours::getColour(item.trackIndex));
    g.fillRect(0, 0, 3, height);

    auto bbt = sequence->tickToBarBeatTick(item.tick);

    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    int x = 6;
    g.drawText(formatPosition(bbt), x, 0, colPosition, height, juce::Justification::centredLeft);
    x += colPosition;
    g.drawText(formatEvent(item), x, 0, colEvent, height, juce::Justification::centredLeft);
    x += colEvent;

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(formatLength(item), x, 0, colLength, height, juce::Justification::centredLeft);
    x += colLength;
    g.drawText(formatValue(item), x, 0, colValue, height, juce::Justification::centredLeft);
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
        return juce::String((item.data2 << 7) | item.data1);
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
