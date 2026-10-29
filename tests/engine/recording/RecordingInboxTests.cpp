#include "engine/recording/RecordingInbox.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <variant>
#include <vector>

namespace
{
constexpr double ticksPerMsAt120 = 0.96;
constexpr LoopRange noLoop{};
constexpr LoopRange oneBarLoop{true, 0, 1920};

juce::MidiMessage stampedAt(juce::MidiMessage message, double timeMs)
{
    message.setTimeStamp(timeMs / 1000.0);
    return message;
}

juce::MidiMessage noteOnAt(double timeMs)
{
    return stampedAt(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), timeMs);
}

std::vector<ClockPosition> inputPositions(const std::vector<InboxItem>& items)
{
    std::vector<ClockPosition> positions;
    for (const auto& item : items)
        if (const auto* input = std::get_if<RecordedInput>(&item))
            positions.push_back(input->position);
    return positions;
}

std::vector<std::pair<int, int>> boundaries(const std::vector<InboxItem>& items)
{
    std::vector<std::pair<int, int>> found;
    for (const auto& item : items)
        if (const auto* boundary = std::get_if<PassBoundary>(&item))
            found.push_back({boundary->pass, boundary->closeTick});
    return found;
}

std::vector<PassBoundary> boundaryItems(const std::vector<InboxItem>& items)
{
    std::vector<PassBoundary> found;
    for (const auto& item : items)
        if (const auto* boundary = std::get_if<PassBoundary>(&item))
            found.push_back(*boundary);
    return found;
}
std::vector<std::string> itemKinds(const std::vector<InboxItem>& items)
{
    std::vector<std::string> kinds;
    for (const auto& item : items)
    {
        if (const auto* input = std::get_if<RecordedInput>(&item))
            kinds.push_back((input->countIn ? "count-in " : "input ") + std::to_string(input->position.pass) + ":" +
                            std::to_string(input->position.tick));
        else if (const auto* end = std::get_if<CountInEnd>(&item))
            kinds.push_back("end " + std::to_string(end->startTick));
        else
            kinds.push_back("boundary " + std::to_string(std::get<PassBoundary>(item).pass));
    }
    return kinds;
}

std::vector<CountInEnd> countInEnds(const std::vector<InboxItem>& items)
{
    std::vector<CountInEnd> found;
    for (const auto& item : items)
        if (const auto* end = std::get_if<CountInEnd>(&item))
            found.push_back(*end);
    return found;
}
} // namespace

TEST_CASE("the segment starts at the playhead of the clock", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    CHECK(inbox.beginSegment(1010.0) == ClockPosition{1, 9});
}

TEST_CASE("input outside a segment is not queued", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.receive("a", noteOnAt(1005.0));
    inbox.beginSegment(1010.0);
    CHECK(inbox.takeItems().items.empty());

    inbox.endSegment(1020.0);
    inbox.receive("a", noteOnAt(1025.0));
    CHECK(inbox.takeItems().items.empty());
}

TEST_CASE("input stamped before the segment start is dropped", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1010.0);
    inbox.receive("a", noteOnAt(1005.0));
    inbox.receive("a", noteOnAt(1012.0));
    CHECK(inputPositions(inbox.takeItems().items) == std::vector<ClockPosition>{{1, 11}});
}

TEST_CASE("system messages are not queued", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", stampedAt(juce::MidiMessage::midiClock(), 1005.0));
    inbox.receive("a", stampedAt(juce::MidiMessage::midiStart(), 1005.0));
    CHECK(inbox.takeItems().items.empty());
}

TEST_CASE("a loop wrap queues a boundary at the loop end", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1900, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.loopWrapped(1025.0, 4.0, ticksPerMsAt120, oneBarLoop);
    CHECK(boundaries(inbox.takeItems().items) == std::vector<std::pair<int, int>>{{2, 1920}});
}

TEST_CASE("input past the loop end announces the wrap before itself, once", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    inbox.loopWrapped(1021.0, 10.0, ticksPerMsAt120, oneBarLoop);
    inbox.receive("a", noteOnAt(1031.0));

    const auto items = inbox.takeItems().items;
    REQUIRE(items.size() == 3);
    CHECK(std::holds_alternative<PassBoundary>(items[0]));
    CHECK(boundaries(items) == std::vector<std::pair<int, int>>{{2, 1920}});
    CHECK(inputPositions(items) == std::vector<ClockPosition>{{2, 9}, {2, 19}});
}

TEST_CASE("a jump queues a boundary at the tick before the jump", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 960, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.jumped(1010.0, 970, 0, ticksPerMsAt120, noLoop);
    inbox.receive("a", noteOnAt(1005.0));

    const auto items = inbox.takeItems().items;
    CHECK(boundaries(items) == std::vector<std::pair<int, int>>{{2, 970}});
    CHECK(inputPositions(items) == std::vector<ClockPosition>{{2, 0}});
}

TEST_CASE("wraps and jumps outside a segment queue nothing but still count passes", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.loopWrapped(1020.0, 9.0, ticksPerMsAt120, oneBarLoop);
    inbox.jumped(1030.0, 18, 480, ticksPerMsAt120, oneBarLoop);
    CHECK(inbox.beginSegment(1030.0) == ClockPosition{3, 480});
    const auto taken = inbox.takeItems();
    CHECK(taken.items.empty());
    CHECK(taken.progress.passEnds.empty());
}

TEST_CASE("ending clamps only input from the final pass", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1900, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1015.0));
    inbox.loopWrapped(1021.0, 0.0, ticksPerMsAt120, oneBarLoop);
    inbox.receive("a", noteOnAt(1030.0));

    const auto end = inbox.endSegment(1025.0);
    CHECK(end.position == ClockPosition{2, 3});
    CHECK(inputPositions(end.items) == std::vector<ClockPosition>{{1, 1914}, {2, 3}});
}

TEST_CASE("a halted clock places later input at the halted tick", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 480, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.clockHalted();
    CHECK(inbox.drainAt(2000.0).position == ClockPosition{1, 480});
}

TEST_CASE("updating the clock moves later positions", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.updateClock(1100.0, 96.0, ticksPerMsAt120 * 2.0, noLoop);
    CHECK(inbox.drainAt(1110.0).position == ClockPosition{1, 115});
}

TEST_CASE("draining past the loop end announces the wrap the clock has not taken yet", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    const auto drained = inbox.drainAt(1020.0);
    CHECK(drained.position == ClockPosition{2, 9});
    CHECK(boundaries(drained.items) == std::vector<std::pair<int, int>>{{2, 1920}});

    inbox.loopWrapped(1021.0, 10.0, ticksPerMsAt120, oneBarLoop);
    CHECK(inbox.takeItems().items.empty());
}

TEST_CASE("ending past the loop end announces the wrap before handing over the items", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    const auto end = inbox.endSegment(1020.0);
    CHECK(end.position == ClockPosition{2, 9});
    CHECK(boundaries(end.items) == std::vector<std::pair<int, int>>{{2, 1920}});
}

TEST_CASE("stopping right after an announced wrap ends in the announced pass", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    inbox.clockHalted();
    const auto end = inbox.endSegment(1020.0);
    CHECK(end.position == ClockPosition{2, 0});
    CHECK(inputPositions(end.items) == std::vector<ClockPosition>{{2, 0}});
}

TEST_CASE("a jump after an announced wrap closes the announced pass at the loop start", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    inbox.jumped(1000.0, 1910, 480, ticksPerMsAt120, oneBarLoop);
    CHECK(boundaries(inbox.takeItems().items) == std::vector<std::pair<int, int>>{{2, 1920}, {3, 0}});
}

TEST_CASE("handing over the queue reports where playback has sent events, not the extrapolated playhead",
          "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.updateClock(1100.0, 96.6, ticksPerMsAt120, noLoop);
    CHECK(inbox.takeItems().progress.sent == ClockPosition{1, 96});

    const auto drained = inbox.drainAt(1110.0);
    CHECK(drained.position == ClockPosition{1, 106});
    CHECK(drained.progress.sent == ClockPosition{1, 96});

    const auto end = inbox.endSegment(1120.0);
    CHECK(end.position == ClockPosition{1, 115});
    CHECK(end.progress.sent == ClockPosition{1, 96});
}

TEST_CASE("the sent position moves into the next pass only when the clock wraps", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    CHECK(inbox.takeItems().progress.sent == ClockPosition{1, 1910});

    inbox.loopWrapped(1021.0, 10.5, ticksPerMsAt120, oneBarLoop);
    CHECK(inbox.takeItems().progress.sent == ClockPosition{2, 10});
}

TEST_CASE("leaving a pass records where playback stopped sending it, once", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1900, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.loopWrapped(1025.0, 4.0, ticksPerMsAt120, oneBarLoop);
    inbox.jumped(1030.0, 8, 480, ticksPerMsAt120, oneBarLoop);
    CHECK(inbox.takeItems().progress.passEnds == std::vector<ClockPosition>{{1, 1920}, {2, 8}});
    CHECK(inbox.takeItems().progress.passEnds.empty());
}

TEST_CASE("a jump that cancels an announced wrap ends the pass where playback stopped", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    inbox.jumped(1000.0, 1910, 480, ticksPerMsAt120, oneBarLoop);

    const auto taken = inbox.takeItems();
    CHECK(boundaries(taken.items) == std::vector<std::pair<int, int>>{{2, 1920}, {3, 0}});
    CHECK(taken.progress.passEnds == std::vector<ClockPosition>{{1, 1910}});
}

TEST_CASE("input carries the loop it was placed in", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1010.0));

    const auto items = inbox.takeItems().items;
    REQUIRE(items.size() == 1);
    CHECK(std::get<RecordedInput>(items[0]).loop == oneBarLoop);
}

TEST_CASE("a loop wrap starts the next pass at the loop start", "[engine][recording]")
{
    constexpr LoopRange loop{true, 480, 1920};
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1900, ticksPerMsAt120, loop);
    inbox.beginSegment(1000.0);
    inbox.loopWrapped(1025.0, 484.0, ticksPerMsAt120, loop);
    CHECK(boundaryItems(inbox.takeItems().items) ==
          std::vector<PassBoundary>{{.pass = 2, .closeTick = 1920, .loopWrap = true, .startTick = 480}});
}

TEST_CASE("a wrap announced by input starts the next pass at the loop start", "[engine][recording]")
{
    constexpr LoopRange loop{true, 480, 1920};
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, loop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    CHECK(boundaryItems(inbox.takeItems().items) ==
          std::vector<PassBoundary>{{.pass = 2, .closeTick = 1920, .loopWrap = true, .startTick = 480}});
}

TEST_CASE("a jump starts the next pass at its target", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 960, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.jumped(1010.0, 970, 480, ticksPerMsAt120, noLoop);
    CHECK(boundaryItems(inbox.takeItems().items) ==
          std::vector<PassBoundary>{{.pass = 2, .closeTick = 970, .loopWrap = false, .startTick = 480}});
}

TEST_CASE("input played during the count-in is marked and placed at its end", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    CHECK(inbox.beginSegment(1000.0) == ClockPosition{1, 1920});
    inbox.receive("a", noteOnAt(1500.0));
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"count-in 1:1920"});
}

TEST_CASE("the end of the count-in is queued once, before input played after it", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(2990.0));
    inbox.receive("a", noteOnAt(3010.0));
    inbox.countInEnded();
    CHECK(itemKinds(inbox.takeItems().items) ==
          std::vector<std::string>{"count-in 1:1920", "end 1920", "input 1:1929"});
}

TEST_CASE("input stamped exactly at the count-in end is ordinary input", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(3000.0));
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"end 1920", "input 1:1920"});
}

TEST_CASE("the timer queues the end of the count-in once when no input crosses it", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.countInEnded();
    inbox.countInEnded();
    inbox.receive("a", noteOnAt(3010.0));
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"end 1920", "input 1:1929"});
}

TEST_CASE("the end of the count-in carries the loop", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 960, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.countInEnded();
    CHECK(countInEnds(inbox.takeItems().items) == std::vector<CountInEnd>{{.startTick = 960, .loop = oneBarLoop}});
}

TEST_CASE("a jump during the count-in moves where it ends", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    CHECK(inbox.countInUpdated(960, ticksPerMsAt120, noLoop));
    inbox.receive("a", noteOnAt(1500.0));
    inbox.countInEnded();
    inbox.receive("a", noteOnAt(3010.0));
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"count-in 1:960", "end 960", "input 1:969"});
}

TEST_CASE("input past the end of the count-in follows the tempo and loop of the latest update",
          "[engine][recording][count-in]")
{
    constexpr LoopRange twoBarLoop{true, 0, 3840};
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    CHECK(inbox.countInUpdated(1920, 2 * ticksPerMsAt120, twoBarLoop));
    inbox.receive("a", noteOnAt(3010.0));
    const auto items = inbox.takeItems().items;
    CHECK(itemKinds(items) == std::vector<std::string>{"end 1920", "input 1:1939"});
    CHECK(countInEnds(items) == std::vector<CountInEnd>{{.startTick = 1920, .loop = twoBarLoop}});
}

TEST_CASE("an update after input has ended the count-in is refused", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(3010.0));
    CHECK_FALSE(inbox.countInUpdated(960, ticksPerMsAt120, noLoop));
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"end 1920", "input 1:1929"});
}

TEST_CASE("input during a pause in the count-in stays count-in input past the old end", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.clockHalted();
    inbox.receive("a", noteOnAt(3500.0));
    inbox.countInDelayed(4000.0);
    inbox.receive("a", noteOnAt(3900.0));
    inbox.receive("a", noteOnAt(4010.0));
    CHECK(itemKinds(inbox.takeItems().items) ==
          std::vector<std::string>{"count-in 1:1920", "count-in 1:1920", "end 1920", "input 1:1929"});
}

TEST_CASE("the end of a count-in before the segment starts is not queued", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.countInEnded();
    inbox.beginSegment(3100.0);
    inbox.receive("a", noteOnAt(3110.0));
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"input 1:2025"});
}

TEST_CASE("starting playback again forgets an unfinished count-in", "[engine][recording][count-in]")
{
    RecordingInbox inbox;
    inbox.countInStarted(3000.0, 1920, ticksPerMsAt120, noLoop);
    inbox.clockHalted();
    inbox.playbackStarted(5000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(5000.0);
    inbox.receive("a", noteOnAt(5010.0));
    inbox.countInEnded();
    CHECK(itemKinds(inbox.takeItems().items) == std::vector<std::string>{"input 2:9"});
}
