#include "edit/ControllerPointEdits.h"
#include "model/MidiSequence.h"
#include "support/ControllerPointTestHelpers.h"
#include "undo/ReplaceEventsAction.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

namespace
{
MidiTrack trackWith(const Events& events)
{
    MidiTrack track;
    for (const auto& event : events)
        track.addEvent(event);
    return track;
}
} // namespace

TEST_CASE("replacing events is undone and redone", "[undo][controller]")
{
    const Events before{p(0, 0), other(240)};
    const Events after{p(0, 0), p(480, 64), other(240)};
    MidiSequence seq;
    seq.addTrack();
    const TrackId id = seq.addTrack(trackWith(before)).getId();
    ReplaceEventsAction action(&seq, id, before, after);

    action.perform();
    CHECK(seq.getTrack(1).getEvents() == after);

    action.undo();
    CHECK(seq.getTrack(1).getEvents() == before);

    action.perform();
    CHECK(seq.getTrack(1).getEvents() == after);
}

TEST_CASE("undo brings back the points a move deleted", "[undo][controller]")
{
    const Events before{p(0, 0), p(480, 40), p(960, 80), p(1440, 40)};
    const Events after = ControllerPointEdits::move(before, cc1, {1}, 1, 1200, 40).events;
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith(before)).getId();
    ReplaceEventsAction action(&seq, id, before, after);

    action.perform();
    action.undo();

    CHECK(seq.getTrack(0).getEvents() == before);
}
