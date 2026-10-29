#include "engine/chase/Chase.h"
#include "engine/chase/ChaseScan.h"
#include <algorithm>
#include <utility>

namespace
{
struct ScannedTarget
{
    ChaseTarget target;
    PlaybackTrackContext firstCtx;
    ChaseScan scan;
};

bool programIsSatisfiedBy(const ChaseItem& program, const std::vector<ChaseItem>& sent)
{
    const auto sentProgram = std::find_if(sent.begin(), sent.end(),
                                          [](const ChaseItem& item) { return item.kind == ChaseItem::Kind::Program; });
    return sentProgram != sent.end() &&
           std::all_of(program.events.begin(), program.events.end(),
                       [&](const MidiEvent& event)
                       {
                           return std::find(sentProgram->events.begin(), sentProgram->events.end(), event) !=
                                  sentProgram->events.end();
                       });
}
} // namespace

std::vector<ChaseTargetState> chaseStates(const PlaybackSnapshot& snap, int tick)
{
    std::vector<ScannedTarget> scanned;
    for (const auto& se : snap.events)
    {
        if (se.event.tick >= tick)
            break;
        if (se.ctx.destination == MidiTrack::OutputDestination::None)
            continue;
        const ChaseTarget target = chaseTargetOf(se.ctx);
        auto it = std::find_if(scanned.begin(), scanned.end(), [&](const auto& s) { return s.target == target; });
        if (it == scanned.end())
            it = scanned.insert(scanned.end(), {target, se.ctx, ChaseScan{}});
        it->scan.apply(se.event);
    }

    std::vector<ChaseTargetState> states;
    for (const auto& s : scanned)
    {
        auto items = s.scan.items(tick);
        if (!items.empty())
            states.push_back({s.target, s.firstCtx, std::move(items)});
    }
    return states;
}

std::vector<ScheduledEvent> chaseEvents(const PlaybackSnapshot& snap, int tick)
{
    std::vector<ScheduledEvent> events;
    for (const auto& state : chaseStates(snap, tick))
        for (const auto& item : state.items)
            for (const auto& event : item.events)
                events.push_back({state.ctx, event});
    return events;
}

std::vector<ChaseTarget> chaseTargetsChangedBetween(const std::vector<PlaybackTrackContext>& before,
                                                    const std::vector<PlaybackTrackContext>& after)
{
    std::vector<ChaseTarget> targets;
    const auto addMissing =
        [&](const std::vector<PlaybackTrackContext>& from, const std::vector<PlaybackTrackContext>& other)
    {
        for (const auto& ctx : from)
        {
            if (ctx.destination == MidiTrack::OutputDestination::None ||
                std::find(other.begin(), other.end(), ctx) != other.end())
                continue;
            const ChaseTarget target = chaseTargetOf(ctx);
            if (std::find(targets.begin(), targets.end(), target) == targets.end())
                targets.push_back(target);
        }
    };
    addMissing(after, before);
    addMissing(before, after);
    return targets;
}

std::vector<ScheduledEvent> chaseChanges(const ChaseTracker& sent, const std::vector<PlaybackTrackContext>& before,
                                         const PlaybackSnapshot& after, int tick)
{
    const auto contextsChanged = chaseTargetsChangedBetween(before, after.contexts);
    auto changed = contextsChanged;
    const auto afterStates = chaseStates(after, tick);
    for (const auto& state : afterStates)
    {
        if (std::find(changed.begin(), changed.end(), state.target) != changed.end() || state.items.empty() ||
            state.items.front().kind != ChaseItem::Kind::Program)
            continue;
        if (!programIsSatisfiedBy(state.items.front(), sent.itemsOf(state.target, tick)))
            changed.push_back(state.target);
    }
    if (changed.empty())
        return {};

    std::vector<ScheduledEvent> events;
    for (const auto& target : changed)
    {
        const auto now = std::find_if(afterStates.begin(), afterStates.end(),
                                      [&](const ChaseTargetState& s) { return s.target == target; });
        if (now == afterStates.end())
            continue;
        const auto old = sent.itemsOf(target, tick);
        const auto differs = [&](const ChaseItem& item)
        {
            const auto previous = std::find_if(old.begin(), old.end(), [&](const ChaseItem& i)
                                               { return i.kind == item.kind && i.number == item.number; });
            return previous == old.end() || *previous != item;
        };

        const bool contextsUnchanged =
            std::find(contextsChanged.begin(), contextsChanged.end(), target) == contextsChanged.end();
        const bool programChanged =
            contextsUnchanged ||
            (!now->items.empty() && now->items.front().kind == ChaseItem::Kind::Program && differs(now->items.front()));
        const auto hasPendingBank = [](const std::vector<ChaseItem>& items)
        {
            return std::ranges::any_of(items,
                                       [](const ChaseItem& i) { return i.kind == ChaseItem::Kind::PendingBank; });
        };
        // The instrument still holds the bank select that is no longer pending, which would apply to the next program
        // change; the bank of the current program puts it back.
        const bool restoresProgramBank = !programChanged && hasPendingBank(old) && !hasPendingBank(now->items);
        bool parameterSent = false;
        for (const auto& item : now->items)
        {
            if (item.kind == ChaseItem::Kind::Program && restoresProgramBank)
                for (auto event = item.events.begin(); event + 1 < item.events.end(); ++event)
                    events.push_back({now->ctx, *event});
            const bool restoresSelection = item.kind == ChaseItem::Kind::Selection && parameterSent;
            if (!programChanged && !restoresSelection && !differs(item))
                continue;
            parameterSent |= item.kind == ChaseItem::Kind::Parameter;
            for (const auto& event : item.events)
                events.push_back({now->ctx, event});
        }
    }
    return events;
}
