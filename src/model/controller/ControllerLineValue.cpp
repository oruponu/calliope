#include "model/controller/ControllerLineValue.h"
#include "model/controller/ControllerRamps.h"

std::optional<int> controllerLineValueAt(const std::vector<MidiEvent>& events, const ControllerKind& kind, int tick)
{
    const MidiEvent* before = nullptr;
    const MidiEvent* after = nullptr;
    for (const auto& event : events)
    {
        if (!kind.matches(event))
            continue;
        if (event.tick <= tick)
        {
            if (before == nullptr || event.tick >= before->tick)
                before = &event;
        }
        else if (after == nullptr || event.tick < after->tick)
        {
            after = &event;
        }
    }
    if (before == nullptr)
        return std::nullopt;

    const int value = kind.valueOf(*before);
    if (after == nullptr || !after->ramp || !kind.canRamp())
        return value;
    return rampValueAt(before->tick, value, after->tick, kind.valueOf(*after), tick);
}
