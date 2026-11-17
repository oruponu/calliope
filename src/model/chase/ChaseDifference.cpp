#include "model/chase/ChaseDifference.h"
#include <algorithm>

std::vector<MidiEvent> chaseEventsToReach(const std::vector<ChaseItem>& sent, const std::vector<ChaseItem>& target,
                                          bool resendAll)
{
    const auto differs = [&](const ChaseItem& item)
    {
        const auto previous = std::find_if(sent.begin(), sent.end(), [&](const ChaseItem& i)
                                           { return i.kind == item.kind && i.number == item.number; });
        return previous == sent.end() || *previous != item;
    };

    const bool programChanged =
        resendAll || (!target.empty() && target.front().kind == ChaseItem::Kind::Program && differs(target.front()));
    const auto hasPendingBank = [](const std::vector<ChaseItem>& items)
    { return std::ranges::any_of(items, [](const ChaseItem& i) { return i.kind == ChaseItem::Kind::PendingBank; }); };
    // The instrument still holds the bank select that is no longer pending, which would apply to the next program
    // change; the bank of the current program puts it back.
    const bool restoresProgramBank = !programChanged && hasPendingBank(sent) && !hasPendingBank(target);
    std::vector<MidiEvent> events;
    bool parameterSent = false;
    for (const auto& item : target)
    {
        if (item.kind == ChaseItem::Kind::Program && restoresProgramBank)
            for (auto event = item.events.begin(); event + 1 < item.events.end(); ++event)
                events.push_back(*event);
        const bool restoresSelection = item.kind == ChaseItem::Kind::Selection && parameterSent;
        if (!programChanged && !restoresSelection && !differs(item))
            continue;
        parameterSent |= item.kind == ChaseItem::Kind::Parameter;
        events.insert(events.end(), item.events.begin(), item.events.end());
    }
    return events;
}
