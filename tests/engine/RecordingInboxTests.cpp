#include "engine/RecordingInbox.h"
#include <catch2/catch_test_macros.hpp>
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
    CHECK(inbox.takeItems().empty());

    inbox.endSegment(1020.0);
    inbox.receive("a", noteOnAt(1025.0));
    CHECK(inbox.takeItems().empty());
}

TEST_CASE("input stamped before the segment start is dropped", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1010.0);
    inbox.receive("a", noteOnAt(1005.0));
    inbox.receive("a", noteOnAt(1012.0));
    CHECK(inputPositions(inbox.takeItems()) == std::vector<ClockPosition>{{1, 11}});
}

TEST_CASE("system messages are not queued", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 0, ticksPerMsAt120, noLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", stampedAt(juce::MidiMessage::midiClock(), 1005.0));
    inbox.receive("a", stampedAt(juce::MidiMessage::midiStart(), 1005.0));
    CHECK(inbox.takeItems().empty());
}

TEST_CASE("a loop wrap queues a boundary at the loop end", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1900, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.loopWrapped(1025.0, 4.0, ticksPerMsAt120, oneBarLoop);
    CHECK(boundaries(inbox.takeItems()) == std::vector<std::pair<int, int>>{{2, 1920}});
}

TEST_CASE("input past the loop end announces the wrap before itself, once", "[engine][recording]")
{
    RecordingInbox inbox;
    inbox.playbackStarted(1000.0, 1910, ticksPerMsAt120, oneBarLoop);
    inbox.beginSegment(1000.0);
    inbox.receive("a", noteOnAt(1020.0));
    inbox.loopWrapped(1021.0, 10.0, ticksPerMsAt120, oneBarLoop);
    inbox.receive("a", noteOnAt(1031.0));

    const auto items = inbox.takeItems();
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

    const auto items = inbox.takeItems();
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
    CHECK(inbox.takeItems().empty());
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
    CHECK(inbox.takeItems().empty());
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
    CHECK(boundaries(inbox.takeItems()) == std::vector<std::pair<int, int>>{{2, 1920}, {3, 0}});
}
