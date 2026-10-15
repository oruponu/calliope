#include "ui/EventListItems.h"
#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

namespace
{
EventListItem noteItem(const MidiNote& note, int trackIndex, int sourceIndex)
{
    EventListItem item;
    item.kind = EventListItem::Note;
    item.tick = note.startTick;
    item.trackIndex = trackIndex;
    item.sourceIndex = sourceIndex;
    item.noteNumber = note.noteNumber;
    item.duration = note.duration;
    item.velocity = note.velocity;
    return item;
}

EventListItem eventItem(const MidiEvent& event, int trackIndex, int sourceIndex)
{
    EventListItem item;
    item.tick = event.tick;
    item.trackIndex = trackIndex;
    item.sourceIndex = sourceIndex;
    item.data1 = event.data1;
    item.data2 = event.data2;

    switch (event.type)
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
    return item;
}
} // namespace

std::vector<EventListItem> EventListItems::build(const MidiSequence& sequence, const std::set<int>& tracks,
                                                 const RecordingLayer* layer)
{
    std::vector<EventListItem> items;

    for (const int trackIdx : tracks)
    {
        if (trackIdx < 0 || trackIdx >= sequence.getNumTracks())
            continue;

        const auto& track = sequence.getTrack(trackIdx);
        for (int i = 0; i < track.getNumNotes(); ++i)
            items.push_back(noteItem(track.getNote(i), trackIdx, i));
        for (int i = 0; i < track.getNumEvents(); ++i)
            items.push_back(eventItem(track.getEvent(i), trackIdx, i));

        if (layer == nullptr)
            continue;
        const auto& take = layer->getTake();
        const auto recorded = take.find(track.getId());
        if (recorded == take.end())
            continue;
        for (const auto& event : recorded->second.events)
        {
            auto item = eventItem(event, trackIdx, -1);
            item.isRecording = true;
            items.push_back(item);
        }
        const auto& notes = recorded->second.notes;
        const auto& ids = recorded->second.noteIds;
        for (size_t i = 0; i < notes.size() && i < ids.size(); ++i)
        {
            auto item = noteItem(notes[i], trackIdx, ids[i]);
            item.isRecording = true;
            items.push_back(item);
        }
    }

    std::stable_sort(items.begin(), items.end(),
                     [](const EventListItem& a, const EventListItem& b)
                     {
                         if (a.tick != b.tick)
                             return a.tick < b.tick;
                         if (a.trackIndex != b.trackIndex)
                             return a.trackIndex < b.trackIndex;
                         return a.kind < b.kind;
                     });
    return items;
}

NoteRef EventListItems::noteRefOf(const EventListItem& item)
{
    return {item.trackIndex, item.sourceIndex, item.isRecording ? NoteLayer::Recording : NoteLayer::Sequence};
}

int EventListItems::playheadRow(const std::vector<EventListItem>& items, double tick)
{
    if (items.empty())
        return -1;

    auto it = std::upper_bound(items.begin(), items.end(), static_cast<int>(tick),
                               [](int t, const EventListItem& item) { return t < item.tick; });
    if (it == items.begin())
        return 0;

    --it;
    if (tick == static_cast<double>(it->tick))
    {
        auto first = std::lower_bound(items.begin(), it, it->tick,
                                      [](const EventListItem& item, int t) { return item.tick < t; });
        return static_cast<int>(std::distance(items.begin(), first));
    }
    return static_cast<int>(std::distance(items.begin(), it));
}

bool EventListItems::isSameEvent(const EventListItem& a, const EventListItem& b)
{
    return a.kind != EventListItem::Note && a.kind == b.kind && a.trackIndex == b.trackIndex && a.tick == b.tick &&
           a.data1 == b.data1 && a.data2 == b.data2 && a.isRecording == b.isRecording;
}

std::vector<int> EventListItems::reselectedRows(const std::vector<EventListItem>& before,
                                                const std::vector<int>& selected,
                                                const std::vector<EventListItem>& after)
{
    std::set<NoteRef> notes;
    // Each selected event with how many equal events come before it.
    std::vector<std::pair<EventListItem, int>> events;
    for (const int row : selected)
    {
        if (row < 0 || row >= static_cast<int>(before.size()))
            continue;
        const auto& item = before[static_cast<size_t>(row)];
        if (item.kind == EventListItem::Note)
        {
            notes.insert(noteRefOf(item));
            continue;
        }
        const auto earlier = std::ranges::subrange(before.begin(), before.begin() + row);
        const auto rank =
            std::ranges::count_if(earlier, [&item](const EventListItem& other) { return isSameEvent(item, other); });
        events.push_back({item, static_cast<int>(rank)});
    }

    std::vector<int> rows;
    for (int row = 0; row < static_cast<int>(after.size()); ++row)
    {
        const auto& item = after[static_cast<size_t>(row)];
        if (item.kind == EventListItem::Note && notes.contains(noteRefOf(item)))
            rows.push_back(row);
    }
    for (const auto& [event, rank] : events)
    {
        int seen = 0;
        for (int row = 0; row < static_cast<int>(after.size()); ++row)
        {
            if (!isSameEvent(event, after[static_cast<size_t>(row)]))
                continue;
            if (seen++ == rank)
            {
                rows.push_back(row);
                break;
            }
        }
    }
    std::ranges::sort(rows);
    return rows;
}
