#include "edit/ProgramChangeEdits.h"
#include "edit/ControllerPointEdits.h"
#include "model/controller/ControllerKind.h"
#include <algorithm>
#include <map>
#include <ranges>

namespace
{
std::vector<int> eventIndicesInTickOrder(const std::vector<MidiEvent>& events)
{
    std::vector<int> indices;
    indices.reserve(events.size());
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        indices.push_back(i);
    std::ranges::stable_sort(indices, {}, [&events](int i) { return events[static_cast<size_t>(i)].tick; });
    return indices;
}

std::vector<int> programChangeIndices(const std::vector<MidiEvent>& events)
{
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        if (events[static_cast<size_t>(i)].type == MidiEvent::Type::ProgramChange)
            indices.push_back(i);
    std::ranges::stable_sort(indices, {}, [&events](int i) { return events[static_cast<size_t>(i)].tick; });
    return indices;
}

bool isBankSelect(const MidiEvent& event)
{
    return event.type == MidiEvent::Type::ControlChange && (event.data1 == 0 || event.data1 == 32);
}

void applyBankSelect(ProgramBank& bank, const MidiEvent& event)
{
    if (event.data1 == 0)
        bank.msb = event.data2;
    else if (event.data1 == 32)
        bank.lsb = event.data2;
}

struct Entry
{
    MidiEvent event;
    bool selected = false;
};

bool isProgramChange(const MidiEvent& event)
{
    return event.type == MidiEvent::Type::ProgramChange;
}

std::set<int> programChangesIn(const std::vector<MidiEvent>& events, const std::set<int>& indices)
{
    std::set<int> programs;
    for (int i : indices)
        if (i >= 0 && i < static_cast<int>(events.size()) && isProgramChange(events[static_cast<size_t>(i)]))
            programs.insert(i);
    return programs;
}

// ControllerPointEdits returns events without source indices, so data2 carries each index through edits that leave it
// alone. restoreEvent then restores the original value.
std::vector<MidiEvent> taggedEvents(const std::vector<MidiEvent>& events)
{
    auto tagged = events;
    for (int i = 0; i < static_cast<int>(tagged.size()); ++i)
        tagged[static_cast<size_t>(i)].data2 = -i - 1;
    return tagged;
}

int tagOf(const MidiEvent& event)
{
    return -event.data2 - 1;
}

MidiEvent restoreEvent(const MidiEvent& transformed, const std::vector<MidiEvent>& before)
{
    auto event = before[static_cast<size_t>(tagOf(transformed))];
    event.tick = transformed.tick;
    event.data1 = transformed.data1;
    return event;
}

std::vector<MidiEvent> movedBankSelects(const std::vector<MidiEvent>& before, int pcIndex, int tick)
{
    std::vector<MidiEvent> events;
    const auto indices = ProgramChangeEdits::bankSelectsOf(before, pcIndex);
    for (int number : {0, 32})
    {
        for (int i : indices)
        {
            auto event = before[static_cast<size_t>(i)];
            if (event.data1 == number)
            {
                event.tick = tick;
                events.push_back(event);
            }
        }
    }
    return events;
}

struct PlacedItem
{
    std::vector<MidiEvent> events;
    bool group = false;
    bool selected = false;
};

bool isWaitingBankSelect(const PlacedItem& item)
{
    return !item.group && item.events.size() == 1 && isBankSelect(item.events.front());
}

ControllerPointEditResult arrangePlacedItems(const std::vector<PlacedItem>& items)
{
    std::vector<const PlacedItem*> arranged;
    for (size_t first = 0; first < items.size();)
    {
        size_t last = first + 1;
        while (last < items.size() && items[last].events.front().tick == items[first].events.front().tick)
            ++last;

        auto waiting = last;
        for (size_t i = first; i < last; ++i)
        {
            if (isWaitingBankSelect(items[i]))
            {
                waiting = i;
                break;
            }
        }

        for (size_t i = first; i < waiting; ++i)
            arranged.push_back(&items[i]);
        for (size_t i = waiting; i < last; ++i)
            if (items[i].group)
                arranged.push_back(&items[i]);
        for (size_t i = waiting; i < last; ++i)
            if (!items[i].group)
                arranged.push_back(&items[i]);
        first = last;
    }

    ControllerPointEditResult result;
    for (const auto* item : arranged)
    {
        const int firstIndex = static_cast<int>(result.events.size());
        result.events.insert(result.events.end(), item->events.begin(), item->events.end());
        if (item->selected)
            result.selected.insert(firstIndex + static_cast<int>(item->events.size()) - 1);
    }
    return result;
}

} // namespace

std::optional<int> ProgramChangeEdits::blockTickAt(const std::vector<MidiEvent>& events, int tick)
{
    std::optional<int> blockTick;
    for (int i : programChangeIndices(events))
    {
        const int eventTick = events[static_cast<size_t>(i)].tick;
        if (eventTick > tick)
            break;
        blockTick = eventTick;
    }
    return blockTick;
}

std::set<int> ProgramChangeEdits::onTick(const std::vector<MidiEvent>& events, int tick)
{
    std::set<int> indices;
    for (int i : programChangeIndices(events))
    {
        const int eventTick = events[static_cast<size_t>(i)].tick;
        if (eventTick > tick)
            break;
        if (eventTick == tick)
            indices.insert(i);
    }
    return indices;
}

std::set<int> ProgramChangeEdits::bankSelectsOf(const std::vector<MidiEvent>& events, int pcIndex)
{
    if (pcIndex < 0 || pcIndex >= static_cast<int>(events.size()))
        return {};
    const auto& programChange = events[static_cast<size_t>(pcIndex)];
    if (programChange.type != MidiEvent::Type::ProgramChange)
        return {};

    const auto programs = onTick(events, programChange.tick);
    if (programs.empty() || pcIndex != *programs.rbegin())
        return {};

    std::set<int> bankSelects;
    for (int i = 0; i < pcIndex; ++i)
    {
        const auto& event = events[static_cast<size_t>(i)];
        if (event.tick == programChange.tick && isBankSelect(event))
            bankSelects.insert(i);
    }
    return bankSelects;
}

std::set<int> ProgramChangeEdits::withBankSelects(const std::vector<MidiEvent>& events, const std::set<int>& pcIndices)
{
    auto indices = pcIndices;
    for (int pcIndex : pcIndices)
    {
        const auto bankSelects = bankSelectsOf(events, pcIndex);
        indices.insert(bankSelects.begin(), bankSelects.end());
    }
    return indices;
}

std::map<int, ProgramVoice> ProgramChangeEdits::voicesOf(const std::vector<MidiEvent>& events)
{
    std::map<int, ProgramVoice> voices;
    ProgramBank bank;
    for (int i : eventIndicesInTickOrder(events))
    {
        const auto& event = events[static_cast<size_t>(i)];
        if (isBankSelect(event))
            applyBankSelect(bank, event);
        else if (event.type == MidiEvent::Type::ProgramChange)
            voices.emplace(i, ProgramVoice{bank, event.data1});
    }
    return voices;
}

ProgramVoice ProgramChangeEdits::voiceOf(const std::vector<MidiEvent>& events, int pcIndex)
{
    const auto voices = voicesOf(events);
    const auto voice = voices.find(pcIndex);
    return voice == voices.end() ? ProgramVoice{} : voice->second;
}

ProgramVoice ProgramChangeEdits::voiceAt(const std::vector<MidiEvent>& events, int tick)
{
    ProgramBank bank;
    ProgramVoice voice;
    for (int i : eventIndicesInTickOrder(events))
    {
        const auto& event = events[static_cast<size_t>(i)];
        if (event.tick > tick)
            break;
        if (isBankSelect(event))
            applyBankSelect(bank, event);
        else if (event.type == MidiEvent::Type::ProgramChange)
            voice = {bank, event.data1};
    }
    return voice;
}

ProgramBank ProgramChangeEdits::bankBefore(const std::vector<MidiEvent>& events, int tick)
{
    ProgramBank bank;
    for (int i : eventIndicesInTickOrder(events))
    {
        const auto& event = events[static_cast<size_t>(i)];
        if (event.tick >= tick)
            break;
        if (isBankSelect(event))
            applyBankSelect(bank, event);
    }
    return bank;
}

ControllerPointEditResult ProgramChangeEdits::place(const std::vector<MidiEvent>& before, int tick,
                                                    const ProgramVoice& voice)
{
    const auto replaced = onTick(before, tick);
    const int kept = replaced.empty() ? -1 : *replaced.rbegin();
    const auto removedBankSelects = kept < 0 ? std::set<int>{} : bankSelectsOf(before, kept);
    const bool writeBank = !voice.bank.isZero() || !bankBefore(before, tick).isZero();

    int insertion = kept;
    if (insertion < 0)
    {
        for (int i = 0; i < static_cast<int>(before.size()); ++i)
        {
            const auto& event = before[static_cast<size_t>(i)];
            if (event.tick == tick && isBankSelect(event))
            {
                insertion = i;
                break;
            }
        }
    }

    std::vector<Entry> entries;
    entries.reserve(before.size() + 3);
    const auto insertVoice = [&]
    {
        if (writeBank)
        {
            entries.push_back({ControllerKind::controlChange(0).point(tick, voice.bank.msb)});
            entries.push_back({ControllerKind::controlChange(32).point(tick, voice.bank.lsb)});
        }
        entries.push_back({ControllerKind::programChange().point(tick, voice.program), true});
    };

    for (int i = 0; i < static_cast<int>(before.size()); ++i)
    {
        if (i == insertion)
            insertVoice();
        if (replaced.contains(i) || removedBankSelects.contains(i))
            continue;
        entries.push_back({before[static_cast<size_t>(i)]});
    }
    if (insertion < 0)
        insertVoice();

    std::ranges::stable_sort(entries, {}, [](const Entry& entry) { return entry.event.tick; });
    ControllerPointEditResult result;
    result.events.reserve(entries.size());
    for (const auto& entry : entries)
    {
        if (entry.selected)
            result.selected.insert(static_cast<int>(result.events.size()));
        result.events.push_back(entry.event);
    }
    return result;
}

ControllerPointEditResult ProgramChangeEdits::move(const std::vector<MidiEvent>& before, const std::set<int>& moved,
                                                   int anchorIndex, int targetTick)
{
    const auto programs = programChangesIn(before, moved);
    if (!programs.contains(anchorIndex))
    {
        const auto base = ControllerPointEdits::move(taggedEvents(before), ControllerKind::programChange(), programs,
                                                     anchorIndex, targetTick, 0);
        std::vector<PlacedItem> items;
        for (const auto& event : base.events)
            items.push_back({{restoreEvent(event, before)}});
        return arrangePlacedItems(items);
    }

    const int anchorValue = before[static_cast<size_t>(anchorIndex)].data1;
    const auto base = ControllerPointEdits::move(taggedEvents(before), ControllerKind::programChange(), programs,
                                                 anchorIndex, targetTick, anchorValue);

    std::map<int, int> survivingProgramTicks;
    for (const auto& event : base.events)
        if (isProgramChange(event))
            survivingProgramTicks.emplace(tagOf(event), event.tick);

    std::set<int> affectedPrograms;
    std::set<int> changedPrograms;
    for (int i : programChangeIndices(before))
    {
        const auto found = survivingProgramTicks.find(i);
        if (found == survivingProgramTicks.end())
        {
            affectedPrograms.insert(i);
        }
        else if (found->second != before[static_cast<size_t>(i)].tick)
        {
            affectedPrograms.insert(i);
            changedPrograms.insert(i);
        }
    }
    auto removedBankSelects = withBankSelects(before, affectedPrograms);
    for (int i : affectedPrograms)
        removedBankSelects.erase(i);

    std::vector<PlacedItem> items;
    for (int i = 0; i < static_cast<int>(base.events.size()); ++i)
    {
        const auto& transformed = base.events[static_cast<size_t>(i)];
        const int originalIndex = tagOf(transformed);
        if (removedBankSelects.contains(originalIndex))
            continue;

        auto event = restoreEvent(transformed, before);
        if (!changedPrograms.contains(originalIndex))
        {
            items.push_back({{event}, false, base.selected.contains(i)});
            continue;
        }

        auto group = movedBankSelects(before, originalIndex, event.tick);
        group.push_back(event);
        items.push_back({std::move(group), true, base.selected.contains(i)});
    }
    return arrangePlacedItems(items);
}

ControllerPointEditResult ProgramChangeEdits::remove(const std::vector<MidiEvent>& before, const std::set<int>& removed)
{
    const auto programs = programChangesIn(before, removed);
    return ControllerPointEdits::remove(before, withBankSelects(before, programs));
}

CopiedControllerPoints ProgramChangeEdits::copy(const std::vector<MidiEvent>& events, const std::set<int>& selected)
{
    CopiedControllerPoints copied{ControllerKind::programChange(), {}};
    for (int i : programChangesIn(events, selected))
    {
        const auto& event = events[static_cast<size_t>(i)];
        CopiedControllerPoint point{event.tick, event.data1, event.ramp};
        for (int bankIndex : bankSelectsOf(events, i))
        {
            const auto& bank = events[static_cast<size_t>(bankIndex)];
            if (bank.data1 == 0)
                point.bankMsb = bank.data2;
            else
                point.bankLsb = bank.data2;
        }
        copied.points.push_back(point);
    }
    if (!copied.points.empty())
    {
        const int firstTick = copied.points.front().tick;
        for (auto& point : copied.points)
            point.tick -= firstTick;
    }
    return copied;
}

ControllerPointEditResult ProgramChangeEdits::paste(const std::vector<MidiEvent>& before,
                                                    const std::vector<CopiedControllerPoint>& points, int atTick)
{
    auto pastedPoints = points;
    for (auto& point : pastedPoints)
        point.ramp = false;
    const auto base =
        ControllerPointEdits::paste(taggedEvents(before), ControllerKind::programChange(), pastedPoints, atTick);

    std::set<int> survivingPrograms;
    for (const auto& event : base.events)
        if (isProgramChange(event) && event.data2 < 0)
            survivingPrograms.insert(tagOf(event));

    std::set<int> deletedPrograms;
    for (int i : programChangeIndices(before))
        if (!survivingPrograms.contains(i))
            deletedPrograms.insert(i);
    const auto removedBankSelects = withBankSelects(before, deletedPrograms);

    size_t pastedIndex = 0;
    std::vector<PlacedItem> items;
    for (int i = 0; i < static_cast<int>(base.events.size()); ++i)
    {
        const auto& transformed = base.events[static_cast<size_t>(i)];
        if (transformed.data2 < 0)
        {
            const int originalIndex = tagOf(transformed);
            if (!removedBankSelects.contains(originalIndex))
                items.push_back({{restoreEvent(transformed, before)}, false, base.selected.contains(i)});
            continue;
        }

        const auto& point = pastedPoints[pastedIndex++];
        std::vector<MidiEvent> group;
        if (point.bankMsb.has_value())
            group.push_back(ControllerKind::controlChange(0).point(transformed.tick, *point.bankMsb));
        if (point.bankLsb.has_value())
            group.push_back(ControllerKind::controlChange(32).point(transformed.tick, *point.bankLsb));
        group.push_back(transformed);
        items.push_back({std::move(group), true, base.selected.contains(i)});
    }
    return arrangePlacedItems(items);
}
