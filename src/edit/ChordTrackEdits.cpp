#include "edit/ChordTrackEdits.h"
#include <algorithm>

namespace
{
std::vector<ChordChange> normalizeNoChordRuns(const std::vector<ChordChange>& changes)
{
    std::vector<ChordChange> result;
    result.reserve(changes.size());
    for (const auto& cc : changes)
    {
        if (cc.isNoChord() && (result.empty() || result.back().isNoChord()))
            continue;
        result.push_back(cc);
    }
    return result;
}
} // namespace

void ChordTrackEdits::add(std::vector<ChordChange>& changes, int tick, int chordRoot, int chordType, int bassRoot,
                          int bassType)
{
    for (auto& cc : changes)
    {
        if (cc.tick == tick)
        {
            cc.chordRoot = chordRoot;
            cc.chordType = chordType;
            cc.bassRoot = bassRoot;
            cc.bassType = bassType;
            return;
        }
    }
    changes.push_back({tick, chordRoot, chordType, bassRoot, bassType});
    std::ranges::sort(changes, {}, &ChordChange::tick);
}

std::pair<int, int> ChordTrackEdits::addSpanAt(const std::vector<ChordChange>& chords, int tick,
                                               const TimelineMap& timeline)
{
    if (tick < 0)
        return {0, 0};

    int governing = -1;
    for (int i = 0; i < static_cast<int>(chords.size()); ++i)
    {
        if (chords[static_cast<size_t>(i)].tick > tick)
            break;
        governing = i;
    }

    if (governing >= 0 && !chords[static_cast<size_t>(governing)].isNoChord())
        return {0, 0};

    const int bar = timeline.tickToBarBeatTick(tick).bar;
    const int barStart = timeline.barStartToTick(bar);
    const int nextBarStart = timeline.barStartToTick(bar + 1);

    int start = barStart;
    if (governing >= 0)
        start = std::max(chords[static_cast<size_t>(governing)].tick, barStart);

    int end = nextBarStart;
    for (const auto& cc : chords)
    {
        if (cc.tick > start)
        {
            end = std::min(end, cc.tick);
            break;
        }
    }

    return {start, end};
}

std::vector<ChordChange> ChordTrackEdits::afterAdd(const std::vector<ChordChange>& before, int startTick, int endTick,
                                                   int chordRoot, int chordType, int bassRoot, int bassType)
{
    if (endTick <= startTick)
        return before;

    auto changes = before;

    auto upsert = [&changes](const ChordChange& entry)
    {
        for (auto& cc : changes)
        {
            if (cc.tick == entry.tick)
            {
                cc = entry;
                return;
            }
        }
        changes.push_back(entry);
        std::ranges::sort(changes, {}, &ChordChange::tick);
    };

    upsert({startTick, chordRoot, chordType, bassRoot, bassType});

    auto next = std::ranges::find_if(changes, [startTick](const ChordChange& cc) { return cc.tick > startTick; });
    if (next == changes.end() || next->tick > endTick)
        upsert(ChordChange::noChord(endTick));

    return changes;
}

std::vector<ChordChange> ChordTrackEdits::afterResize(const std::vector<ChordChange>& before, int chordIndex,
                                                      int targetEndTick, int gridTicks)
{
    if (chordIndex < 0 || chordIndex >= static_cast<int>(before.size()) || gridTicks <= 0)
        return before;
    if (before[static_cast<size_t>(chordIndex)].isNoChord())
        return before;

    const size_t bodyIndex = static_cast<size_t>(chordIndex);
    const int chordTick = before[bodyIndex].tick;
    int end = ((std::max(0, targetEndTick) + gridTicks / 2) / gridTicks) * gridTicks;
    const int minEnd = (chordTick / gridTicks) * gridTicks + gridTicks;
    end = std::max(end, minEnd);

    auto changes = before;
    const size_t nextIndex = bodyIndex + 1;

    bool rolled = false;
    if (nextIndex < before.size())
    {
        if (end == before[nextIndex].tick)
            return before;
        if (!before[nextIndex].isNoChord() && end < before[nextIndex].tick)
        {
            changes[nextIndex].tick = end;
            rolled = true;
        }
    }

    if (!rolled)
    {
        ChordChange terminator = ChordChange::noChord(end);
        if (nextIndex < before.size() && before[nextIndex].isNoChord())
        {
            terminator = before[nextIndex];
            terminator.tick = end;
        }

        bool tailFound = false;
        ChordChange tail{};
        for (size_t i = nextIndex; i < before.size(); ++i)
        {
            const auto& cc = before[i];
            if (cc.tick >= end || cc.isNoChord())
                continue;
            tailFound = (i + 1 >= before.size()) || before[i + 1].tick > end;
            tail = cc;
        }

        std::erase_if(changes,
                      [chordTick, end](const ChordChange& cc) { return cc.tick > chordTick && cc.tick < end; });
        if (tailFound)
        {
            tail.tick = end;
            changes.push_back(tail);
        }
        else if (std::ranges::none_of(changes, [end](const ChordChange& cc) { return cc.tick == end; }))
        {
            changes.push_back(terminator);
        }
        std::ranges::sort(changes, {}, &ChordChange::tick);
    }

    return normalizeNoChordRuns(changes);
}

std::vector<ChordChange> ChordTrackEdits::afterMove(const std::vector<ChordChange>& before,
                                                    const std::vector<int>& movedIndices, int anchorIndex,
                                                    int targetTick, int gridTicks)
{
    if (gridTicks <= 0)
        return before;

    std::vector<int> moving;
    for (int i : movedIndices)
        if (i >= 0 && i < static_cast<int>(before.size()) && !before[static_cast<size_t>(i)].isNoChord())
            moving.push_back(i);
    std::ranges::sort(moving);
    moving.erase(std::unique(moving.begin(), moving.end()), moving.end());
    if (moving.empty() || !std::ranges::binary_search(moving, anchorIndex))
        return before;

    const int anchorTick = before[static_cast<size_t>(anchorIndex)].tick;
    const int snapped = ((std::max(0, targetTick) + gridTicks / 2) / gridTicks) * gridTicks;
    const int delta = std::max(snapped - anchorTick, -before[static_cast<size_t>(moving.front())].tick);
    if (delta == 0)
        return before;

    struct MovedContent
    {
        ChordChange body{};
        bool open = false;
        bool hasTerminator = false;
        ChordChange terminator{};
        int length = 0;
    };
    std::vector<MovedContent> contents;
    std::vector<bool> removed(before.size(), false);
    for (int i : moving)
    {
        MovedContent mc;
        mc.body = before[static_cast<size_t>(i)];
        removed[static_cast<size_t>(i)] = true;
        const size_t n = static_cast<size_t>(i) + 1;
        if (n >= before.size())
        {
            mc.open = true;
        }
        else
        {
            mc.length = before[n].tick - mc.body.tick;
            mc.hasTerminator = before[n].isNoChord();
            if (mc.hasTerminator)
            {
                mc.terminator = before[n];
                removed[n] = true;
            }
        }
        contents.push_back(mc);
    }

    std::vector<ChordChange> changes;
    for (size_t i = 0; i < before.size(); ++i)
        if (!removed[i])
            changes.push_back(before[i]);

    for (int i : moving)
    {
        const size_t p = static_cast<size_t>(i);
        if (p > 0 && !removed[p - 1] && !before[p - 1].isNoChord())
            changes.push_back(ChordChange::noChord(before[p].tick));
    }
    std::ranges::sort(changes, {}, &ChordChange::tick);

    for (size_t k = 0; k < contents.size();)
    {
        if (contents[k].open)
        {
            ++k;
            continue;
        }
        size_t last = k;
        while (!contents[last].hasTerminator && last + 1 < contents.size() && !contents[last + 1].open &&
               moving[last + 1] == moving[last] + 1)
            ++last;

        const int startTick = contents[k].body.tick + delta;
        const int endTick = contents[last].body.tick + contents[last].length + delta;

        bool tailFound = false;
        ChordChange tail{};
        for (size_t i = 0; i < before.size(); ++i)
        {
            const auto& cc = before[i];
            if (removed[i] || cc.tick < startTick || cc.tick >= endTick || cc.isNoChord())
                continue;
            tailFound = (i + 1 >= before.size()) || before[i + 1].tick > endTick;
            tail = cc;
        }
        std::erase_if(changes, [startTick, endTick](const ChordChange& cc)
                      { return cc.tick >= startTick && cc.tick < endTick; });
        if (tailFound)
        {
            tail.tick = endTick;
            changes.push_back(tail);
            std::ranges::sort(changes, {}, &ChordChange::tick);
        }
        k = last + 1;
    }

    for (const auto& mc : contents)
    {
        if (!mc.open)
            continue;
        const int newTick = mc.body.tick + delta;
        std::erase_if(changes, [newTick](const ChordChange& cc) { return cc.tick == newTick; });
    }

    for (const auto& mc : contents)
    {
        ChordChange moved = mc.body;
        moved.tick = mc.body.tick + delta;
        changes.push_back(moved);
    }
    std::ranges::sort(changes, {}, &ChordChange::tick);

    for (const auto& mc : contents)
    {
        if (mc.open)
            continue;
        const int endTick = mc.body.tick + mc.length + delta;
        if (std::ranges::any_of(changes, [endTick](const ChordChange& cc) { return cc.tick == endTick; }))
            continue;
        if (mc.hasTerminator)
        {
            ChordChange terminator = mc.terminator;
            terminator.tick = endTick;
            changes.push_back(terminator);
        }
        else
        {
            changes.push_back(ChordChange::noChord(endTick));
        }
    }
    std::ranges::sort(changes, {}, &ChordChange::tick);

    return normalizeNoChordRuns(changes);
}

std::vector<ChordChange> ChordTrackEdits::afterStartResize(const std::vector<ChordChange>& before, int chordIndex,
                                                           int targetStartTick, int gridTicks)
{
    if (chordIndex < 0 || chordIndex >= static_cast<int>(before.size()) || gridTicks <= 0)
        return before;
    if (before[static_cast<size_t>(chordIndex)].isNoChord())
        return before;

    const size_t bodyIndex = static_cast<size_t>(chordIndex);
    const ChordChange body = before[bodyIndex];
    const int target = std::max(0, targetStartTick);
    if (target == body.tick)
        return before;

    int newTick = ((target + gridTicks / 2) / gridTicks) * gridTicks;
    if (target > body.tick)
    {
        if (newTick < body.tick)
            newTick = (body.tick / gridTicks) * gridTicks + gridTicks;
        if (bodyIndex + 1 < before.size())
            newTick = std::min(newTick, ((before[bodyIndex + 1].tick - 1) / gridTicks) * gridTicks);
        if (newTick <= body.tick)
            return before;
    }
    else
    {
        if (newTick > body.tick)
            newTick = ((body.tick - 1) / gridTicks) * gridTicks;
        if (newTick >= body.tick)
            return before;
    }

    auto changes = before;
    changes.erase(changes.begin() + static_cast<std::ptrdiff_t>(bodyIndex));

    if (newTick < body.tick)
    {
        const int oldTick = body.tick;
        std::erase_if(changes,
                      [newTick, oldTick](const ChordChange& cc) { return cc.tick >= newTick && cc.tick < oldTick; });
    }

    ChordChange moved = body;
    moved.tick = newTick;
    changes.push_back(moved);
    std::ranges::sort(changes, {}, &ChordChange::tick);
    return changes;
}

std::vector<ChordChange> ChordTrackEdits::afterDelete(const std::vector<ChordChange>& before,
                                                      const std::vector<int>& deletedIndices)
{
    std::vector<int> deleting;
    for (int i : deletedIndices)
        if (i >= 0 && i < static_cast<int>(before.size()) && !before[static_cast<size_t>(i)].isNoChord())
            deleting.push_back(i);
    std::ranges::sort(deleting);
    deleting.erase(std::unique(deleting.begin(), deleting.end()), deleting.end());
    if (deleting.empty())
        return before;

    std::vector<bool> removed(before.size(), false);
    for (int i : deleting)
    {
        removed[static_cast<size_t>(i)] = true;
        const size_t n = static_cast<size_t>(i) + 1;
        if (n < before.size() && before[n].isNoChord())
            removed[n] = true;
    }

    std::vector<ChordChange> changes;
    for (size_t i = 0; i < before.size(); ++i)
        if (!removed[i])
            changes.push_back(before[i]);

    for (int i : deleting)
    {
        const size_t p = static_cast<size_t>(i);
        if (p > 0 && !removed[p - 1] && !before[p - 1].isNoChord())
            changes.push_back(ChordChange::noChord(before[p].tick));
    }
    std::ranges::sort(changes, {}, &ChordChange::tick);

    return normalizeNoChordRuns(changes);
}

std::vector<ChordChange> ChordTrackEdits::afterPaste(const std::vector<ChordChange>& before,
                                                     const std::vector<RelativeChord>& items, int anchorTick)
{
    std::vector<RelativeChord> sorted;
    for (const auto& item : items)
        if (item.tickOffset >= 0 && item.length >= 0)
            sorted.push_back(item);
    std::ranges::sort(sorted, {}, &RelativeChord::tickOffset);
    sorted.erase(std::unique(sorted.begin(), sorted.end(), [](const RelativeChord& a, const RelativeChord& b)
                             { return a.tickOffset == b.tickOffset; }),
                 sorted.end());
    if (sorted.empty())
        return before;

    auto changes = before;

    for (size_t k = 0; k < sorted.size();)
    {
        if (sorted[k].length == 0)
        {
            ++k;
            continue;
        }
        size_t last = k;
        while (last + 1 < sorted.size() && sorted[last + 1].length != 0 &&
               sorted[last].tickOffset + sorted[last].length == sorted[last + 1].tickOffset)
            ++last;

        const int startTick = anchorTick + sorted[k].tickOffset;
        const int endTick = anchorTick + sorted[last].tickOffset + sorted[last].length;

        bool tailFound = false;
        ChordChange tail{};
        for (size_t i = 0; i < before.size(); ++i)
        {
            const auto& cc = before[i];
            if (cc.tick < startTick || cc.tick >= endTick || cc.isNoChord())
                continue;
            tailFound = (i + 1 >= before.size()) || before[i + 1].tick > endTick;
            tail = cc;
        }
        std::erase_if(changes, [startTick, endTick](const ChordChange& cc)
                      { return cc.tick >= startTick && cc.tick < endTick; });
        if (tailFound)
        {
            tail.tick = endTick;
            changes.push_back(tail);
            std::ranges::sort(changes, {}, &ChordChange::tick);
        }
        k = last + 1;
    }

    for (const auto& item : sorted)
    {
        if (item.length != 0)
            continue;
        const int newTick = anchorTick + item.tickOffset;
        std::erase_if(changes, [newTick](const ChordChange& cc) { return cc.tick == newTick; });
    }

    for (const auto& item : sorted)
        changes.push_back({anchorTick + item.tickOffset, item.chordRoot, item.chordType, item.bassRoot, item.bassType});
    std::ranges::sort(changes, {}, &ChordChange::tick);

    for (const auto& item : sorted)
    {
        if (item.length == 0)
            continue;
        const int endTick = anchorTick + item.tickOffset + item.length;
        if (std::ranges::any_of(changes, [endTick](const ChordChange& cc) { return cc.tick == endTick; }))
            continue;
        changes.push_back(ChordChange::noChord(endTick));
    }
    std::ranges::sort(changes, {}, &ChordChange::tick);

    return normalizeNoChordRuns(changes);
}
