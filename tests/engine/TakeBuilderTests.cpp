#include "engine/TakeBuilder.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
constexpr TrackId piano{1};
constexpr TrackId bass{2};

juce::MidiMessage noteOn(int note, int velocity = 90, int channel = 1)
{
    return juce::MidiMessage::noteOn(channel, note, static_cast<juce::uint8>(velocity));
}

juce::MidiMessage noteOff(int note, int channel = 1)
{
    return juce::MidiMessage::noteOff(channel, note);
}

juce::MidiMessage cc(int controller, int value, int channel = 1)
{
    return juce::MidiMessage::controllerEvent(channel, controller, value);
}

MidiEvent ccEvent(int tick, int controller, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = controller, .data2 = value};
}

std::vector<MidiNote> notesOn(const TakeBuilder& builder, TrackId track)
{
    const auto& take = builder.getTake();
    const auto it = take.find(track);
    return it == take.end() ? std::vector<MidiNote>{} : it->second.notes;
}

std::vector<MidiEvent> eventsOn(const Take& take, TrackId track)
{
    const auto it = take.find(track);
    return it == take.end() ? std::vector<MidiEvent>{} : it->second.events;
}

std::vector<MidiEvent> eventsOn(const TakeBuilder& builder, TrackId track)
{
    return eventsOn(builder.getTake(), track);
}

TakeBuilder startedOn(std::optional<TrackId> target)
{
    TakeBuilder builder;
    builder.begin(target, {1, 0});
    return builder;
}
} // namespace

TEST_CASE("a note is recorded from its note-on to its note-off", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 100, 480}});
}

TEST_CASE("a note-on with velocity zero ends the note", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOn(60, 0), {1, 340});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 100, 240}});
}

TEST_CASE("a note-off for a key pressed before the segment is dropped", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOff(60), {1, 100});
    CHECK(builder.getTake().empty());
}

TEST_CASE("a held note is shown until it is released", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 100}});
    builder.handle("a", noteOff(60), {1, 200});
    CHECK(builder.getHeldNotes().empty());
}

TEST_CASE("a second note-on for a held key ends the first note there", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 0});
    builder.handle("a", noteOn(60, 70), {1, 240});
    builder.handle("a", noteOff(60), {1, 480});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 240}, {60, 70, 240, 240}});
}

TEST_CASE("a note that ends on its start tick lasts one tick", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 100});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 100, 1}});
}

TEST_CASE("the same key from two sources makes two notes", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 0});
    builder.handle("b", noteOn(60, 80), {1, 100});
    builder.handle("a", noteOff(60), {1, 200});
    builder.handle("b", noteOff(60), {1, 300});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 200}, {60, 80, 100, 200}});
}

TEST_CASE("ending the segment closes held notes at the end tick and clears the take", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 0});
    const auto take = builder.end(480);
    REQUIRE(take.contains(piano));
    CHECK(take.at(piano).notes == std::vector<MidiNote>{{60, 90, 0, 480}});
    CHECK(builder.getTake().empty());
    CHECK(builder.getHeldNotes().empty());
}

TEST_CASE("controller, program, pitch bend and pressure messages are recorded as events", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(7, 100), {1, 10});
    builder.handle("a", juce::MidiMessage::programChange(1, 5), {1, 20});
    builder.handle("a", juce::MidiMessage::pitchWheel(1, 12000), {1, 30});
    builder.handle("a", juce::MidiMessage::channelPressureChange(1, 64), {1, 40});
    builder.handle("a", juce::MidiMessage::aftertouchChange(1, 60, 50), {1, 50});
    CHECK(eventsOn(builder, piano) ==
          std::vector<MidiEvent>{ccEvent(10, 7, 100),
                                 {.type = MidiEvent::Type::ProgramChange, .tick = 20, .data1 = 5},
                                 {.type = MidiEvent::Type::PitchBend, .tick = 30, .data1 = 12000},
                                 {.type = MidiEvent::Type::ChannelPressure, .tick = 40, .data1 = 64},
                                 {.type = MidiEvent::Type::KeyPressure, .tick = 50, .data1 = 60, .data2 = 50}});
}

TEST_CASE("system messages are not recorded", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", juce::MidiMessage::midiClock(), {1, 10});
    builder.handle("a", juce::MidiMessage::midiStop(), {1, 20});
    CHECK(builder.getTake().empty());
}

TEST_CASE("input from an earlier pass is dropped", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.closeHeld(2, 1920);
    builder.handle("a", noteOn(60), {1, 1919});
    builder.handle("a", cc(7, 100), {1, 1919});
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}

TEST_CASE("a pass boundary closes held notes at the boundary tick", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1800});
    builder.closeHeld(2, 1920);
    builder.handle("a", noteOff(60), {2, 10});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 1800, 120}});
}

TEST_CASE("switching the target closes held notes on the old track", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 0});
    builder.setTarget(bass, 240);
    builder.handle("a", noteOff(60), {1, 300});
    builder.handle("a", noteOn(40), {1, 400});
    builder.handle("a", noteOff(40), {1, 500});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 240}});
    CHECK(notesOn(builder, bass) == std::vector<MidiNote>{{40, 90, 400, 100}});
}

TEST_CASE("input without a target is not recorded", "[engine][take]")
{
    auto builder = startedOn(std::nullopt);
    builder.handle("a", noteOn(60), {1, 0});
    builder.handle("a", noteOff(60), {1, 100});
    builder.handle("a", cc(7, 100), {1, 100});
    CHECK(builder.getTake().empty());
}

TEST_CASE("a pedal held at the end is released at the end tick", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 100});
    const auto take = builder.end(480);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(100, 64, 127), ccEvent(480, 64, 0)});
}

TEST_CASE("a pedal released before the end gets no extra release", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 100});
    builder.handle("a", cc(64, 0), {1, 200});
    const auto take = builder.end(480);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(100, 64, 127), ccEvent(200, 64, 0)});
}

TEST_CASE("a pedal pressed while there was no target gets no release", "[engine][take]")
{
    auto builder = startedOn(std::nullopt);
    builder.handle("a", cc(64, 127), {1, 50});
    builder.setTarget(piano, 100);
    const auto take = builder.end(480);
    CHECK(take.empty());
}

TEST_CASE("switching the target releases the pedal on the old track", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 100});
    builder.setTarget(bass, 240);
    const auto take = builder.end(480);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(100, 64, 127), ccEvent(240, 64, 0)});
    CHECK(eventsOn(take, bass).empty());
}

TEST_CASE("a pass boundary releases a held pedal one tick before the boundary", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 100});
    builder.closeHeld(2, 1920);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(100, 64, 127), ccEvent(1919, 64, 0)});
}

TEST_CASE("a pedal pressed on the boundary tick is released after its press", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 1920});
    builder.closeHeld(2, 1920);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(1920, 64, 127), ccEvent(1920, 64, 0)});
}

TEST_CASE("sostenuto and hold 2 are released like sustain", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(66, 127), {1, 10});
    builder.handle("a", cc(69, 100), {1, 20});
    const auto take = builder.end(480);
    CHECK(eventsOn(take, piano) ==
          std::vector<MidiEvent>{ccEvent(10, 66, 127), ccEvent(20, 69, 100), ccEvent(480, 66, 0), ccEvent(480, 69, 0)});
}

TEST_CASE("releasing a source closes its notes and the pedal it holds", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 0});
    builder.handle("b", noteOn(62), {1, 0});
    builder.handle("a", cc(64, 127), {1, 50});
    builder.releaseSource("a", 300);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 300}});
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(50, 64, 127), ccEvent(300, 64, 0)});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 62, 90, 0}});
}

TEST_CASE("releasing a source that does not hold the pedal leaves it down", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 50});
    builder.releaseSource("b", 300);
    const auto take = builder.end(480);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(50, 64, 127), ccEvent(480, 64, 0)});
}

TEST_CASE("releasing one of two sources holding the pedal releases it on the track", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 10});
    builder.handle("b", cc(64, 127, 2), {1, 20});
    builder.releaseSource("a", 300);
    const auto take = builder.end(480);
    CHECK(eventsOn(take, piano) ==
          std::vector<MidiEvent>{ccEvent(10, 64, 127), ccEvent(20, 64, 127), ccEvent(300, 64, 0)});
}

TEST_CASE("the content revision changes only when the take changes", "[engine][take]")
{
    auto builder = startedOn(piano);
    const int started = builder.getContentRevision();
    builder.handle("a", noteOn(60), {1, 0});
    CHECK(builder.getContentRevision() == started);
    builder.handle("a", noteOff(60), {1, 100});
    CHECK(builder.getContentRevision() != started);
}

TEST_CASE("an event changes the content revision but not the note revision", "[engine][take]")
{
    auto builder = startedOn(piano);
    const int content = builder.getContentRevision();
    const int notes = builder.getNoteRevision();
    builder.handle("a", juce::MidiMessage::pitchWheel(1, 9000), {1, 10});
    CHECK(builder.getContentRevision() != content);
    CHECK(builder.getNoteRevision() == notes);
    builder.handle("a", noteOn(60), {1, 20});
    builder.handle("a", noteOff(60), {1, 30});
    CHECK(builder.getNoteRevision() != notes);
}
