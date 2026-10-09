#include "engine/ThruRouter.h"
#include <catch2/catch_test_macros.hpp>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace
{
const PlaybackTrackContext pianoTrack{TrackId{1}, 2, TrackId{1}, MidiTrack::OutputDestination::Plugin};
const PlaybackTrackContext bassTrack{TrackId{2}, 5, TrackId{2}, MidiTrack::OutputDestination::MidiDevice};

std::string describe(TrackId track, const juce::MidiMessage& message)
{
    std::string text = std::format("t{}:", std::to_underlying(track));
    const auto* data = message.getRawData();
    for (int i = 0; i < message.getRawDataSize(); ++i)
        text += std::format(" {:02x}", static_cast<int>(data[i]));
    return text;
}

class RecordingSink : public LiveMidiSink
{
public:
    std::vector<std::string> sent;
    std::vector<double> timestamps;

    void sendLive(const PlaybackTrackContext& ctx, const juce::MidiMessage& message) override
    {
        sent.push_back(describe(ctx.trackId, message));
        timestamps.push_back(message.getTimeStamp());
    }
};

juce::MidiMessage noteOn(int channel, int note, int velocity = 100)
{
    return juce::MidiMessage::noteOn(channel, note, static_cast<juce::uint8>(velocity));
}

juce::MidiMessage noteOff(int channel, int note)
{
    return juce::MidiMessage::noteOff(channel, note);
}

juce::MidiMessage cc(int channel, int controller, int value)
{
    return juce::MidiMessage::controllerEvent(channel, controller, value);
}

using Sent = std::vector<std::string>;
} // namespace

TEST_CASE("note on is sent to the target on the track's channel", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOn(2, 60))});
}

TEST_CASE("channel voice messages are forwarded on the track's channel", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", cc(1, 1, 64), sink);
    router.handle("kbd", juce::MidiMessage::programChange(1, 5), sink);
    router.handle("kbd", juce::MidiMessage::pitchWheel(1, 10000), sink);
    router.handle("kbd", juce::MidiMessage::channelPressureChange(1, 50), sink);
    router.handle("kbd", juce::MidiMessage::aftertouchChange(1, 60, 70), sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, cc(2, 1, 64)),
                            describe(TrackId{1}, juce::MidiMessage::programChange(2, 5)),
                            describe(TrackId{1}, juce::MidiMessage::pitchWheel(2, 10000)),
                            describe(TrackId{1}, juce::MidiMessage::channelPressureChange(2, 50)),
                            describe(TrackId{1}, juce::MidiMessage::aftertouchChange(2, 60, 70))});
}

TEST_CASE("system messages are not forwarded", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    const juce::uint8 sysex[] = {0x7e, 0x7f, 0x09, 0x01};
    router.handle("kbd", juce::MidiMessage::createSysExMessage(sysex, 4), sink);
    router.handle("kbd", juce::MidiMessage::midiClock(), sink);
    router.handle("kbd", juce::MidiMessage::midiStart(), sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("nothing is sent without a target", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("changing target turns off held notes on the previous target", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 64), sink);
    router.handle("kbd", noteOn(1, 60), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOff(2, 60)), describe(TrackId{1}, noteOff(2, 64))});
}

TEST_CASE("changing target releases a held sustain pedal on the previous target", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOff(2, 60)), describe(TrackId{1}, cc(2, 64, 0))});
}

TEST_CASE("sostenuto and hold 2 pedals are released on the previous target", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", cc(1, 69, 100), sink);
    router.handle("kbd", cc(1, 66, 127), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, cc(2, 66, 0)), describe(TrackId{1}, cc(2, 69, 0))});
}

TEST_CASE("pedals let go before the change are not released again", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    router.handle("kbd", cc(1, 64, 63), sink);
    router.handle("kbd", cc(1, 66, 127), sink);
    router.handle("kbd", cc(1, 66, 0), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("controllers that do not hold notes are not reset on target change", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", cc(1, 1, 100), sink);
    router.handle("kbd", cc(1, 67, 127), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("releasing a key after the target changed sends nothing", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.setTarget(bassTrack, 1.0, sink);
    sink.sent.clear();
    router.handle("kbd", noteOff(1, 60), sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("note off for a key that was never pressed is dropped", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOff(1, 60), sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("key pressed while there is no target is not turned off later", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.handle("kbd", noteOn(1, 60), sink);
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOff(1, 60), sink);
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("setting the same target again sends nothing", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    sink.sent.clear();
    router.setTarget(pianoTrack, 1.0, sink);
    router.handle("kbd", noteOff(1, 60), sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOff(2, 60))});
}

TEST_CASE("setting a first target sends nothing even with a pedal held", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.handle("kbd", cc(1, 64, 127), sink);
    router.setTarget(pianoTrack, 0.0, sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("pedal held while there was no target is released when the target changes", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.handle("kbd", cc(1, 64, 127), sink);
    router.setTarget(pianoTrack, 0.0, sink);
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, cc(2, 64, 0))});
}

TEST_CASE("same note number from different sources or channels is tracked separately", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("pad", noteOn(1, 60), sink);
    router.handle("kbd", noteOn(3, 60), sink);
    sink.sent.clear();
    router.handle("kbd", noteOff(1, 60), sink);
    router.handle("pad", noteOff(1, 60), sink);
    router.handle("kbd", noteOff(3, 60), sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOff(2, 60)), describe(TrackId{1}, noteOff(2, 60)),
                            describe(TrackId{1}, noteOff(2, 60))});
}

TEST_CASE("the same note held from two sources is turned off once", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("pad", noteOn(1, 60), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOff(2, 60))});
}

TEST_CASE("note on with velocity zero releases the key", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("kbd", noteOn(1, 60, 0), sink);
    sink.sent.clear();
    router.setTarget(bassTrack, 1.0, sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("releases on target change carry the given time", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    sink.timestamps.clear();
    router.setTarget(bassTrack, 2.5, sink);
    CHECK(sink.timestamps == std::vector<double>{2.5, 2.5});
}

TEST_CASE("pedal value received after the target changed goes to the new target", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    router.setTarget(bassTrack, 1.0, sink);
    sink.sent.clear();
    router.handle("kbd", cc(1, 64, 100), sink);
    CHECK(sink.sent == Sent{describe(TrackId{2}, cc(5, 64, 100))});
}

TEST_CASE("pedal release from one source is forwarded while another source holds its pedal", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    router.handle("pad", cc(1, 64, 127), sink);
    sink.sent.clear();
    router.handle("kbd", cc(1, 64, 0), sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, cc(2, 64, 0))});
}

TEST_CASE("releasing a source turns off only its held notes and pedals", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.handle("kbd", cc(1, 66, 127), sink);
    router.handle("kbd", cc(1, 64, 127), sink);
    router.handle("pad", noteOn(1, 62), sink);
    router.handle("pad", cc(1, 69, 127), sink);
    sink.sent.clear();
    router.releaseSource("kbd", 3.0, sink);
    CHECK(sink.sent == Sent{describe(TrackId{1}, noteOff(2, 60)), describe(TrackId{1}, cc(2, 64, 0)),
                            describe(TrackId{1}, cc(2, 66, 0))});
    CHECK(sink.timestamps.back() == 3.0);
}

TEST_CASE("releasing a source with nothing held sends nothing", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("pad", noteOn(1, 62), sink);
    sink.sent.clear();
    router.releaseSource("kbd", 3.0, sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("key of a released source sends nothing when let go", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.setTarget(pianoTrack, 0.0, sink);
    router.handle("kbd", noteOn(1, 60), sink);
    router.releaseSource("kbd", 3.0, sink);
    sink.sent.clear();
    router.handle("kbd", noteOff(1, 60), sink);
    CHECK(sink.sent.empty());
}

TEST_CASE("releasing a source without a target sends nothing and forgets its pedals", "[engine][thru]")
{
    ThruRouter router;
    RecordingSink sink;
    router.handle("kbd", cc(1, 64, 127), sink);
    router.releaseSource("kbd", 3.0, sink);
    router.setTarget(pianoTrack, 4.0, sink);
    router.setTarget(bassTrack, 5.0, sink);
    CHECK(sink.sent.empty());
}
