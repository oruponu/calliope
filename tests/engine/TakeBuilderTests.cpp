#include "engine/TakeBuilder.h"
#include "support/GridTestHelpers.h"
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

MidiEvent typedEvent(MidiEvent::Type type, int tick, int data1, int data2 = 0)
{
    return {.type = type, .tick = tick, .data1 = data1, .data2 = data2};
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

std::vector<int> idsOn(const TakeBuilder& builder, TrackId track)
{
    const auto& take = builder.getTake();
    const auto it = take.find(track);
    return it == take.end() ? std::vector<int>{} : it->second.noteIds;
}

int heldIdOf(const TakeBuilder& builder, int noteNumber)
{
    for (const auto& held : builder.getHeldNotes())
        if (held.noteNumber == noteNumber)
            return held.id;
    return 0;
}

TakeBuilder startedOn(std::optional<TrackId> target)
{
    TakeBuilder builder;
    builder.begin(target, {1, 0});
    return builder;
}

constexpr LoopRange oneBarLoop{true, 0, 1920};
const TimelineGrid* const sixteenths = &GridTest::fourFourSixteenths;

PassBoundary wrapTo(int pass, int startTick)
{
    return {.pass = pass, .closeTick = 1920, .loopWrap = true, .startTick = startTick};
}

PassBoundary jumpTo(int pass, int closeTick)
{
    return {.pass = pass, .closeTick = closeTick, .loopWrap = false, .startTick = 0};
}

PlaybackProgress wrappedInto(int pass, int sentTick)
{
    return {.sent = {pass, sentTick}, .passEnds = {{pass - 1, 1920}}};
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
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 100, 1}});
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
    builder.closeHeld({.pass = 2, .closeTick = 1920}, {});
    builder.handle("a", noteOn(60), {1, 1919});
    builder.handle("a", cc(7, 100), {1, 1919});
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}

TEST_CASE("a pass boundary closes held notes at the boundary tick", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1800});
    builder.closeHeld({.pass = 2, .closeTick = 1920}, {});
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
    builder.closeHeld({.pass = 2, .closeTick = 1920}, {});
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(100, 64, 127), ccEvent(1919, 64, 0)});
}

TEST_CASE("a pedal pressed on the boundary tick is released after its press", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(64, 127), {1, 1920});
    builder.closeHeld({.pass = 2, .closeTick = 1920}, {});
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
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 62, 90, 0, 2}});
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

TEST_CASE("each note-on gets an id that it keeps after release", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOn(62), {1, 120});
    CHECK(heldIdOf(builder, 60) == 1);
    CHECK(heldIdOf(builder, 62) == 2);
    builder.handle("a", noteOff(62), {1, 200});
    builder.handle("a", noteOff(60), {1, 300});
    CHECK(idsOn(builder, piano) == std::vector<int>{2, 1});
}

TEST_CASE("the next segment does not reuse ids", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.end(200);
    builder.begin(piano, {1, 0});
    builder.handle("a", noteOn(60), {1, 300});
    CHECK(heldIdOf(builder, 60) == 2);
}

TEST_CASE("an edit moves a finished note and keeps its length", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    builder.editNotes({{1, {.startTick = 240, .noteNumber = 64}}}, 1000);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{64, 90, 240, 480}});
}

TEST_CASE("an edit of the start that keeps the end resizes a finished note", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    builder.editNotes({{1, {.startTick = 200, .keepEnd = true}}}, 1000);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 200, 380}});
}

TEST_CASE("an edit of the end resizes a finished note", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    builder.editNotes({{1, {.endTick = 340}}}, 1000);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 100, 240}});
}

TEST_CASE("an edit changes the velocity of a finished note", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    builder.editNotes({{1, {.velocity = 30}}}, 1000);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 30, 100, 480}});
}

TEST_CASE("removing a finished note drops it and its id", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    builder.handle("a", noteOn(62), {1, 600});
    builder.handle("a", noteOff(62), {1, 800});
    builder.removeNotes({1});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{62, 90, 600, 200}});
    CHECK(idsOn(builder, piano) == std::vector<int>{2});
}

TEST_CASE("a held note moved earlier stays held from the new start", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.startTick = 240}}}, 960);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 240, 1}});
    CHECK(notesOn(builder, piano).empty());
    builder.handle("a", noteOff(60), {1, 1200});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 240, 960}});
}

TEST_CASE("a held note moved later but before the playhead stays held", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.startTick = 720}}}, 960);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 720, 1}});
}

TEST_CASE("a held note moved to the playhead or later ends with its length at that moment", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.startTick = 960}}}, 960);
    CHECK(builder.getHeldNotes().empty());
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 960, 480}});
    CHECK(idsOn(builder, piano) == std::vector<int>{1});
}

TEST_CASE("setting the end of a held note ends it there", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.endTick = 720}}}, 960);
    CHECK(builder.getHeldNotes().empty());
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 480, 240}});
}

TEST_CASE("the key of a held note ended by an edit no longer closes a note", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.endTick = 720}}}, 960);
    builder.handle("a", noteOff(60), {1, 1200});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 480, 240}});
}

TEST_CASE("the key of a removed held note no longer closes a note but can record again", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.removeNotes({1});
    builder.handle("a", noteOff(60), {1, 600});
    CHECK(notesOn(builder, piano).empty());
    builder.handle("a", noteOn(60), {1, 700});
    builder.handle("a", noteOff(60), {1, 900});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 700, 200}});
    CHECK(idsOn(builder, piano) == std::vector<int>{2});
}

TEST_CASE("a held note given a new pitch closes at that pitch", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.noteNumber = 67}}}, 600);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 67, 90, 480, 1}});
    builder.handle("a", noteOff(60), {1, 960});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{67, 90, 480, 480}});
}

TEST_CASE("a pass boundary closes a moved held note from its new start", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.startTick = 240}}}, 960);
    builder.closeHeld({.pass = 2, .closeTick = 1920}, {});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 240, 1680}});
    CHECK(idsOn(builder, piano) == std::vector<int>{1});
}

TEST_CASE("edits and removals of unknown ids are ignored", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 100});
    builder.handle("a", noteOff(60), {1, 580});
    builder.editNotes({{99, {.startTick = 0}}}, 0);
    builder.removeNotes({99});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 100, 480}});
}

TEST_CASE("only ending a held note by an edit changes the note revision", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    const int before = builder.getNoteRevision();
    builder.editNotes({{1, {.startTick = 240}}}, 960);
    CHECK(builder.getNoteRevision() == before);
    builder.editNotes({{1, {.startTick = 960}}}, 960);
    CHECK(builder.getNoteRevision() != before);
}

TEST_CASE("a held note moved to the playhead or later can end with a given length", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.startTick = 1200, .heldLength = 240}}}, 960);
    CHECK(builder.getHeldNotes().empty());
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 1200, 240}});
}

TEST_CASE("a held note that stays held ignores the given length", "[engine][take]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 480});
    builder.editNotes({{1, {.startTick = 240, .heldLength = 120}}}, 960);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 240, 1}});
}

TEST_CASE("without a grid a note is shown from where it is played", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 130});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 130, 1}});
}

TEST_CASE("a quantized note starts on the nearest line and keeps its played length", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 130}, sixteenths);
    builder.reach({1, 500});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 120, 1}});
    builder.handle("a", noteOff(60), {1, 400}, sixteenths);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 120, 270}});
}

TEST_CASE("a note released before playback reaches it joins the take once it does", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOff(60), {1, 300}, sixteenths);
    CHECK(builder.getTake().empty());
    builder.reach({1, 241});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 240, 70}});
}

TEST_CASE("a controller is recorded where it is played even with a grid", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", cc(1, 64), {1, 130}, sixteenths);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(130, 1, 64)});
}

TEST_CASE("a pending note is not shown until playback sends its start", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    CHECK_FALSE(builder.reach({1, 235}));
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.reach({1, 241}));
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 240, 1}});
}

TEST_CASE("a pending note is reached only once playback has sent past its start tick", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.reach({1, 240});
    CHECK(builder.getHeldNotes().empty());
    builder.reach({1, 241});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 240, 1}});
}

TEST_CASE("a pending note is reached by a later pass of the clock but not by an earlier one",
          "[engine][take][input-quantize]")
{
    TakeBuilder builder;
    builder.begin(piano, {2, 0});
    builder.handle("a", noteOn(60), {2, 130}, sixteenths);
    builder.reach({1, 1900});
    CHECK(builder.getHeldNotes().empty());
    builder.reach({3, 0});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 120, 1}});
}

TEST_CASE("a quantized note ends no later than the largest tick", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 2147483630}, sixteenths);
    builder.handle("a", noteOff(60), {1, 2147483645}, sixteenths);
    builder.reach({1, 2147483647});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 2147483640, 7}});
}

TEST_CASE("moving a quantized held note drops its played length", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 130}, sixteenths);
    builder.reach({1, 500});
    builder.editNotes({{1, {.startTick = 480}}}, 500);
    builder.handle("a", noteOff(60), {1, 900}, sixteenths);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 480, 420}});
}

TEST_CASE("a new pitch keeps the played length of a quantized held note", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 130}, sixteenths);
    builder.reach({1, 500});
    builder.editNotes({{1, {.noteNumber = 67}}}, 500);
    builder.handle("a", noteOff(60), {1, 900}, sixteenths);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{67, 90, 120, 770}});
}

TEST_CASE("a pending note is neither edited nor removed", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.editNotes({{1, {.startTick = 0}}}, 235);
    builder.removeNotes({1});
    builder.reach({1, 241});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 240, 1}});
}

TEST_CASE("pressing a pending key again ends the pending note with its played length", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOn(60, 70), {1, 235}, sixteenths);
    builder.reach({1, 241});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 240, 5}});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 70, 240, 2}});
}

TEST_CASE("ending the segment drops pending notes, held or released", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOn(62), {1, 350}, sixteenths);
    builder.handle("a", noteOff(62), {1, 355}, sixteenths);
    CHECK(builder.end(236).empty());
}

TEST_CASE("a reached quantized note closed by the end ends at the end tick", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 130}, sixteenths);
    builder.reach({1, 200});
    const auto take = builder.end(600);
    REQUIRE(take.contains(piano));
    CHECK(take.at(piano).notes == std::vector<MidiNote>{{60, 90, 120, 480}});
}

TEST_CASE("a jump drops pending notes, held or released", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOn(62), {1, 350}, sixteenths);
    builder.handle("a", noteOff(62), {1, 355}, sixteenths);
    builder.closeHeld(jumpTo(2, 236), {.sent = {1, 236}});
    builder.reach({2, 1000});
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}

TEST_CASE("a boundary playback has passed keeps the pending notes it sent", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOff(60), {1, 300}, sixteenths);
    builder.handle("a", noteOn(62), {1, 350}, sixteenths);
    builder.closeHeld(jumpTo(2, 500), {.sent = {2, 10}, .passEnds = {{1, 500}}});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 240, 70}, {62, 90, 360, 140}});
}

TEST_CASE("a wrap playback has not taken yet keeps the pending notes it has not sent until it ends the pass",
          "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(62), {1, 1700}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(62), {1, 1710}, sixteenths, oneBarLoop);
    builder.handle("a", noteOn(60), {1, 1790}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(60), {1, 1795}, sixteenths, oneBarLoop);
    builder.closeHeld(wrapTo(2, 0), {.sent = {1, 1780}});
    builder.reach({2, 100});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{62, 90, 1680, 10}});
    CHECK(builder.reachPassEnds({{1, 1920}}));
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{62, 90, 1680, 10}, {60, 90, 1800, 5}});
}

TEST_CASE("a key held across a wrap playback has not taken yet ends at the loop end once playback gets there",
          "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1790}, sixteenths, oneBarLoop);
    builder.closeHeld(wrapTo(2, 0), {.sent = {1, 1780}});
    CHECK(builder.getHeldNotes().empty());
    builder.reachPassEnds({{1, 1920}});
    builder.handle("a", noteOff(60), {2, 100}, sixteenths, oneBarLoop);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 1800, 120}});
}

TEST_CASE("a jump that later cancels an announced wrap keeps only the pending notes playback sent",
          "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1690}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(60), {1, 1695}, sixteenths, oneBarLoop);
    builder.handle("a", noteOn(64), {1, 1790}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(64), {1, 1795}, sixteenths, oneBarLoop);
    builder.closeHeld(wrapTo(2, 0), {.sent = {1, 1670}});
    const PlaybackProgress progress{.sent = {3, 10}, .passEnds = {{1, 1750}}};
    builder.closeHeld(jumpTo(3, 0), progress);
    builder.reach(progress.sent);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 1680, 5}});
}

TEST_CASE("pending notes waiting for the end of a pass are dropped when recording ends first",
          "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1790}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(60), {1, 1795}, sixteenths, oneBarLoop);
    builder.closeHeld(wrapTo(2, 0), {.sent = {1, 1780}});
    builder.reach({2, 0});
    CHECK(builder.end(0).empty());
}

TEST_CASE("a jump that cancels an announced wrap keeps only the pending notes playback sent",
          "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(62), {1, 1700}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(62), {1, 1710}, sixteenths, oneBarLoop);
    builder.handle("a", noteOn(60), {1, 1790}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(60), {1, 1795}, sixteenths, oneBarLoop);
    const PlaybackProgress progress{.sent = {3, 10}, .passEnds = {{1, 1780}}};
    builder.closeHeld(wrapTo(2, 0), progress);
    builder.closeHeld(jumpTo(3, 0), progress);
    builder.reach(progress.sent);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{62, 90, 1680, 10}});
}

TEST_CASE("switching the target drops pending notes, held or released", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOn(62), {1, 350}, sixteenths);
    builder.handle("a", noteOff(62), {1, 355}, sixteenths);
    builder.setTarget(bass, 236);
    builder.reach({1, 1000});
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}

TEST_CASE("releasing a source drops only its pending notes", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 230}, sixteenths);
    builder.handle("a", noteOn(62), {1, 350}, sixteenths);
    builder.handle("a", noteOff(62), {1, 355}, sixteenths);
    builder.handle("b", noteOn(64), {1, 230}, sixteenths);
    builder.releaseSource("a", 236);
    builder.reach({1, 1000});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 64, 90, 240, 3}});
    CHECK(builder.getTake().empty());
}

TEST_CASE("a note quantized to the loop end starts the next pass at the loop start", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1900}, sixteenths, oneBarLoop);
    builder.reach({1, 1919});
    CHECK(builder.getHeldNotes().empty());
    builder.closeHeld(wrapTo(2, 0), wrappedInto(2, 5));
    builder.reach({2, 5});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 0, 1}});
    builder.handle("a", noteOff(60), {2, 100}, sixteenths, oneBarLoop);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 120}});
}

TEST_CASE("a note carried to the next pass and released before the wrap keeps its played length",
          "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1900}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(60), {1, 1915}, sixteenths, oneBarLoop);
    builder.reach({1, 1919});
    CHECK(builder.getTake().empty());
    builder.closeHeld(wrapTo(2, 0), wrappedInto(2, 5));
    builder.reach({2, 5});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 15}});
}

TEST_CASE("a carried note held through the next wrap closes there", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1900}, sixteenths, oneBarLoop);
    builder.closeHeld(wrapTo(2, 0), wrappedInto(2, 5));
    builder.reach({2, 5});
    builder.closeHeld(wrapTo(3, 0), wrappedInto(3, 5));
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 0, 1920}});
}

TEST_CASE("a carried note starts where the loop actually wraps to", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1900}, sixteenths, oneBarLoop);
    builder.closeHeld(wrapTo(2, 480), wrappedInto(2, 485));
    builder.reach({2, 485});
    builder.handle("a", noteOff(60), {2, 600}, sixteenths, oneBarLoop);
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 480, 140}});
}

TEST_CASE("a carried note is dropped when playback jumps instead of wrapping", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1900}, sixteenths, oneBarLoop);
    builder.closeHeld(jumpTo(2, 1910), {.sent = {2, 5}, .passEnds = {{1, 1910}}});
    builder.reach({2, 100});
    builder.handle("a", noteOff(60), {2, 200}, sixteenths, oneBarLoop);
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}

TEST_CASE("a carried note is dropped when recording ends before the wrap", "[engine][take][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.handle("a", noteOn(60), {1, 1900}, sixteenths, oneBarLoop);
    builder.handle("a", noteOff(60), {1, 1915}, sixteenths, oneBarLoop);
    CHECK(builder.end(1918).empty());
}

TEST_CASE("a key released during the count-in is not recorded", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("a", noteOff(60));
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.end(2400).empty());
}

TEST_CASE("a note-on with velocity zero during the count-in releases the key", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("a", noteOn(60, 0));
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes().empty());
}

TEST_CASE("a key held through the count-in starts at its end", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    CHECK(builder.getHeldNotes().empty());
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 1920, 1}});
    builder.handle("a", noteOff(60), {1, 2400});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 1920, 480}});
}

TEST_CASE("a key pressed again after its release during the count-in starts at its end with the last velocity",
          "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60, 50));
    builder.collectCountIn("a", noteOff(60));
    builder.collectCountIn("a", noteOn(60, 110));
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 110, 1920, 1}});
}

TEST_CASE("a key released during the count-in that was pressed before it is ignored", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOff(60));
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.end(2400).empty());
}

TEST_CASE("with input quantize a key held through the count-in starts where its end rounds to",
          "[engine][take][count-in][input-quantize]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.endCountIn(1930, sixteenths);
    builder.handle("a", noteOff(60), {1, 2410}, sixteenths);
    builder.reach({1, 2500});
    CHECK(notesOn(builder, piano) == std::vector<MidiNote>{{60, 90, 1920, 480}});
}

TEST_CASE("only the last value of each controller is recorded at the end of the count-in", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", cc(1, 10));
    builder.collectCountIn("a", cc(1, 20));
    builder.collectCountIn("a", cc(7, 100));
    builder.collectCountIn("a", cc(1, 30));
    builder.endCountIn(1920);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(1920, 7, 100), ccEvent(1920, 1, 30)});
}

TEST_CASE("pitch bend, program change and channel pressure each keep their last value", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", juce::MidiMessage::pitchWheel(1, 1000));
    builder.collectCountIn("a", juce::MidiMessage::programChange(1, 3));
    builder.collectCountIn("a", juce::MidiMessage::pitchWheel(1, 9000));
    builder.collectCountIn("a", juce::MidiMessage::channelPressureChange(1, 40));
    builder.collectCountIn("a", juce::MidiMessage::programChange(1, 5));
    builder.endCountIn(1920);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{typedEvent(MidiEvent::Type::PitchBend, 1920, 9000),
                                                             typedEvent(MidiEvent::Type::ChannelPressure, 1920, 40),
                                                             typedEvent(MidiEvent::Type::ProgramChange, 1920, 5)});
}

TEST_CASE("polyphonic pressure keeps its last value per key and is dropped for released keys",
          "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("a", noteOn(62));
    builder.collectCountIn("a", juce::MidiMessage::aftertouchChange(1, 60, 50));
    builder.collectCountIn("a", juce::MidiMessage::aftertouchChange(1, 62, 70));
    builder.collectCountIn("a", juce::MidiMessage::aftertouchChange(1, 60, 55));
    builder.collectCountIn("a", juce::MidiMessage::channelPressureChange(1, 30));
    builder.collectCountIn("a", noteOff(62));
    builder.endCountIn(1920);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{typedEvent(MidiEvent::Type::KeyPressure, 1920, 60, 55),
                                                             typedEvent(MidiEvent::Type::ChannelPressure, 1920, 30)});
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 60, 90, 1920, 1}});
}

TEST_CASE("values from two inputs are recorded in the order their last values arrived", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", cc(64, 127));
    builder.collectCountIn("b", cc(64, 127));
    builder.collectCountIn("a", cc(64, 0));
    builder.endCountIn(1920);
    CHECK(eventsOn(builder, piano) == std::vector<MidiEvent>{ccEvent(1920, 64, 127), ccEvent(1920, 64, 0)});
}

TEST_CASE("a hold pedal down at the end of the count-in is released when the segment ends", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", cc(64, 127));
    builder.endCountIn(1920);
    const auto take = builder.end(2400);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(1920, 64, 127), ccEvent(2400, 64, 0)});
}

TEST_CASE("a hold pedal below 64 at the end of the count-in is not released when the segment ends",
          "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", cc(64, 40));
    builder.endCountIn(1920);
    const auto take = builder.end(2400);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(1920, 64, 40)});
}

TEST_CASE("a pedal pressed and released during the count-in records the release at its end", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", cc(64, 127));
    builder.collectCountIn("a", cc(64, 0));
    builder.endCountIn(1920);
    const auto take = builder.end(2400);
    CHECK(eventsOn(take, piano) == std::vector<MidiEvent>{ccEvent(1920, 64, 0)});
}

TEST_CASE("a key and the sostenuto pedal played during the count-in both start at its end whichever came first",
          "[engine][take][count-in]")
{
    auto keyFirst = startedOn(piano);
    keyFirst.collectCountIn("a", noteOn(60));
    keyFirst.collectCountIn("a", cc(66, 127));
    keyFirst.endCountIn(1920);
    keyFirst.handle("a", noteOff(60), {1, 2400});
    const auto keyFirstTake = keyFirst.end(2880);

    auto pedalFirst = startedOn(piano);
    pedalFirst.collectCountIn("a", cc(66, 127));
    pedalFirst.collectCountIn("a", noteOn(60));
    pedalFirst.endCountIn(1920);
    pedalFirst.handle("a", noteOff(60), {1, 2400});
    const auto pedalFirstTake = pedalFirst.end(2880);

    for (const auto* take : {&keyFirstTake, &pedalFirstTake})
    {
        CHECK(take->at(piano).notes == std::vector<MidiNote>{{60, 90, 1920, 480}});
        CHECK(eventsOn(*take, piano) == std::vector<MidiEvent>{ccEvent(1920, 66, 127), ccEvent(2880, 66, 0)});
    }
}

TEST_CASE("count-in input goes to the target at the end of the count-in", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("a", cc(64, 127));
    builder.setTarget(bass, 1920);
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{bass, 60, 90, 1920, 1}});
    CHECK(eventsOn(builder, bass) == std::vector<MidiEvent>{ccEvent(1920, 64, 127)});
    CHECK(eventsOn(builder, piano).empty());
}

TEST_CASE("count-in input without a target at the end of the count-in is dropped", "[engine][take][count-in]")
{
    auto builder = startedOn(std::nullopt);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("a", cc(1, 10));
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}

TEST_CASE("releasing an input device drops what it played during the count-in", "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("b", noteOn(62));
    builder.collectCountIn("a", cc(64, 127));
    builder.releaseSource("a", 1920);
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes() == std::vector<HeldTakeNote>{{piano, 62, 90, 1920, 1}});
    CHECK(eventsOn(builder, piano).empty());
}

TEST_CASE("ending the segment during the count-in records nothing and leaves nothing for the next segment",
          "[engine][take][count-in]")
{
    auto builder = startedOn(piano);
    builder.collectCountIn("a", noteOn(60));
    builder.collectCountIn("a", cc(1, 10));
    CHECK(builder.end(1920).empty());

    builder.begin(piano, {2, 0});
    builder.endCountIn(1920);
    CHECK(builder.getHeldNotes().empty());
    CHECK(builder.getTake().empty());
}
