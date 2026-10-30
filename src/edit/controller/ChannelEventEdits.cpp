#include "edit/controller/ChannelEventEdits.h"
#include "model/controller/ControllerPointConversion.h"
#include <algorithm>
#include <utility>

namespace
{
bool sameIgnoringRamp(MidiEvent a, MidiEvent b)
{
    a.ramp = b.ramp = false;
    return a == b;
}

// convertStepsToPoints only puts hold points in, each just before its step and at another value than the step.
int indexAfterConversion(const std::vector<MidiEvent>& steps, const std::vector<MidiEvent>& points, int stepIndex)
{
    int step = 0;
    for (int i = 0; i < static_cast<int>(points.size()); ++i)
    {
        if (!sameIgnoringRamp(points[static_cast<size_t>(i)], steps[static_cast<size_t>(step)]))
            continue;
        if (step == stepIndex)
            return i;
        ++step;
    }
    return -1;
}
} // namespace

ControllerPointEditResult ChannelEventEdits::moveTo(const std::vector<MidiEvent>& before, int index, int tick)
{
    if (index < 0 || index >= static_cast<int>(before.size()))
        return {before, {}};
    const auto holds = unsentControllerPoints(before);
    if (holds[static_cast<size_t>(index)])
        return {before, {}};

    // Moved as a step, so the hold points of every step are made again from the values before them.
    auto steps = convertHoldsToSteps(before);
    const auto stepIndex = index - static_cast<int>(std::count(holds.begin(), holds.begin() + index, true));
    auto event = steps[static_cast<size_t>(stepIndex)];
    steps.erase(steps.begin() + stepIndex);
    event.tick = std::max(0, tick);
    const auto at = steps.insert(std::ranges::upper_bound(steps, event.tick, {}, &MidiEvent::tick), event);
    const int movedIndex = static_cast<int>(at - steps.begin());

    auto points = convertStepsToPoints(steps);
    const int moved = indexAfterConversion(steps, points, movedIndex);
    return {std::move(points), {moved}};
}
