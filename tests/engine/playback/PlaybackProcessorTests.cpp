#include "engine/playback/PlaybackListener.h"
#include "engine/playback/PlaybackProcessor.h"
#include "engine/playback/PlaybackSnapshot.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
struct LoggedCall
{
    enum Kind
    {
        NoteOn,
        NoteOff,
        SysEx,
        Mid
    } kind;
    PlaybackTrackContext ctx;
    MidiNote note;
    MidiEvent event;
    SysExEvent sysEx;
};

class MockListener : public PlaybackListener
{
public:
    std::vector<LoggedCall> log;

    void onNoteOn(const PlaybackTrackContext& c, const MidiNote& n) override
    {
        log.push_back({LoggedCall::NoteOn, c, n, {}, {}});
    }
    void onNoteOff(const PlaybackTrackContext& c, const MidiNote& n) override
    {
        log.push_back({LoggedCall::NoteOff, c, n, {}, {}});
    }
    void onSysEx(const PlaybackTrackContext& c, const SysExEvent& s) override
    {
        log.push_back({LoggedCall::SysEx, c, {}, {}, s});
    }
    void onMidiEvent(const PlaybackTrackContext& c, const MidiEvent& e) override
    {
        log.push_back({LoggedCall::Mid, c, {}, e, {}});
    }
};

MidiEvent controlChange(int tick)
{
    MidiEvent cc;
    cc.type = MidiEvent::Type::ControlChange;
    cc.tick = tick;
    cc.data1 = 7;
    cc.data2 = 100;
    return cc;
}
} // namespace

TEST_CASE("note-on fires when range covers startTick", "[engine][processor]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {60, 100, 100, 200});
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 200, sink);

    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::NoteOn);
    CHECK(sink.log[0].note.noteNumber == 60);
}

TEST_CASE("note-off fires when range passes endTick", "[engine][processor]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {60, 100, 100, 200});
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 200, sink);
    proc.process(snap, 200, 400, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[1].kind == LoggedCall::NoteOff);
}

TEST_CASE("short note that starts and ends within one range is completed in that range", "[engine][processor]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {60, 100, 100, 20});
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 200, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::NoteOn);
    CHECK(sink.log[1].kind == LoggedCall::NoteOff);
    CHECK(sink.log[1].note.noteNumber == 60);
}

TEST_CASE("note-off fires when endTick == toTick", "[engine][processor]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {60, 100, 0, 100});
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 50, sink);
    sink.log.clear();
    proc.process(snap, 50, 100, sink);

    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
}

TEST_CASE("within a range, note-off precedes note-on at the same tick", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 0, 100});
    t.addNote({62, 100, 100, 100});
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 50, sink);
    sink.log.clear();
    proc.process(snap, 50, 150, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[0].note.noteNumber == 60);
    CHECK(sink.log[1].kind == LoggedCall::NoteOn);
    CHECK(sink.log[1].note.noteNumber == 62);
}

TEST_CASE("midi events fire before note-ons within range", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 50, 100});
    t.addEvent(controlChange(20));
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 100, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::Mid);
    CHECK(sink.log[1].kind == LoggedCall::NoteOn);
}

TEST_CASE("expired note-off precedes a midi event in the same range", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 0, 100});
    t.addEvent(controlChange(150));
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 50, sink);
    sink.log.clear();
    proc.process(snap, 50, 200, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[0].note.noteNumber == 60);
    CHECK(sink.log[1].kind == LoggedCall::Mid);
    CHECK(sink.log[1].event.tick == 150);
}

TEST_CASE("midi event is sent before a later note-off in the same range", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 0, 100});
    t.addEvent(controlChange(80));
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 50, sink);
    sink.log.clear();
    proc.process(snap, 50, 200, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::Mid);
    CHECK(sink.log[0].event.tick == 80);
    CHECK(sink.log[1].kind == LoggedCall::NoteOff);
}

TEST_CASE("note-on is sent before a later midi event in the same range", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 50, 1000});
    t.addEvent(controlChange(80));
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 100, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::NoteOn);
    CHECK(sink.log[1].kind == LoggedCall::Mid);
    CHECK(sink.log[1].event.tick == 80);
}

TEST_CASE("midi event between a short note's on and off is sent between them", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 10, 20});
    t.addEvent(controlChange(20));
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 100, sink);

    REQUIRE(sink.log.size() == 3);
    CHECK(sink.log[0].kind == LoggedCall::NoteOn);
    CHECK(sink.log[1].kind == LoggedCall::Mid);
    CHECK(sink.log[2].kind == LoggedCall::NoteOff);
}

TEST_CASE("note-off is sent before a note-on that starts earlier in a later range", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 0, 30});
    t.addNote({62, 100, 40, 100});
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 10, sink);
    sink.log.clear();
    proc.process(snap, 10, 100, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[0].note.noteNumber == 60);
    CHECK(sink.log[1].kind == LoggedCall::NoteOn);
    CHECK(sink.log[1].note.noteNumber == 62);
}

TEST_CASE("note-on is sent before a later note-off of an already sounding note", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 0, 80});
    t.addNote({62, 100, 40, 100});
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 10, sink);
    sink.log.clear();
    proc.process(snap, 10, 100, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::NoteOn);
    CHECK(sink.log[0].note.noteNumber == 62);
    CHECK(sink.log[1].kind == LoggedCall::NoteOff);
    CHECK(sink.log[1].note.noteNumber == 60);
}

TEST_CASE("at the same tick, note-off precedes midi event which precedes note-on", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 0, 50});
    t.addNote({62, 100, 50, 100});
    t.addEvent(controlChange(50));
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 10, sink);
    sink.log.clear();
    proc.process(snap, 10, 100, sink);

    REQUIRE(sink.log.size() == 3);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[1].kind == LoggedCall::Mid);
    CHECK(sink.log[2].kind == LoggedCall::NoteOn);
}

TEST_CASE("at the same tick, note-off precedes sysex which precedes midi event and note-on",
          "[engine][processor][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addNote({62, 100, 480, 100});
    track.addEvent(controlChange(480));
    track.addSysEx({480, {0x01}});
    seq.addTrack(track);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 1, sink);
    sink.log.clear();
    proc.process(snap, 480, 481, sink);

    REQUIRE(sink.log.size() == 4);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[1].kind == LoggedCall::SysEx);
    CHECK(sink.log[2].kind == LoggedCall::Mid);
    CHECK(sink.log[3].kind == LoggedCall::NoteOn);
}

TEST_CASE("at the same tick, every track's sysex precedes every track's midi events", "[engine][processor][sysex]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addEvent(controlChange(0));
    t0.addSysEx({0, {0x01}});
    seq.addTrack(t0);
    MidiTrack t1;
    t1.addSysEx({0, {0x02}});
    seq.addTrack(t1);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 1, sink);

    REQUIRE(sink.log.size() == 3);
    CHECK(sink.log[0].kind == LoggedCall::SysEx);
    CHECK(sink.log[0].sysEx.data == std::vector<std::uint8_t>{0x01});
    CHECK(sink.log[0].ctx.trackId == seq.getTrack(0).getId());
    CHECK(sink.log[1].kind == LoggedCall::SysEx);
    CHECK(sink.log[1].sysEx.data == std::vector<std::uint8_t>{0x02});
    CHECK(sink.log[1].ctx.trackId == seq.getTrack(1).getId());
    CHECK(sink.log[2].kind == LoggedCall::Mid);
}

TEST_CASE("sysex at the start tick is sent and earlier sysex is not", "[engine][processor][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addSysEx({0, {0x01}});
    track.addSysEx({480, {0x02}});
    seq.addTrack(track);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 480);
    proc.process(snap, 480, 481, sink);

    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::SysEx);
    CHECK(sink.log[0].sysEx.data == std::vector<std::uint8_t>{0x02});
}

TEST_CASE("sysex inside a loop is sent again after the loop restarts", "[engine][processor][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addSysEx({240, {0x01}});
    seq.addTrack(track);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 480, sink);
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 480, sink);

    REQUIRE(sink.log.size() == 2);
    CHECK(sink.log[0].kind == LoggedCall::SysEx);
    CHECK(sink.log[1].kind == LoggedCall::SysEx);
}

TEST_CASE("releaseActiveNotesForTrack offs that track's active notes and removes them", "[engine][processor]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {60, 100, 0, 1000});
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {72, 100, 0, 1000});
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 10, sink);
    sink.log.clear();

    proc.releaseActiveNotesForTrack(seq.getTrack(0).getId(), sink);
    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[0].ctx.trackId == seq.getTrack(0).getId());
    CHECK(sink.log[0].note.noteNumber == 60);

    sink.log.clear();
    proc.sendAllNoteOffs(sink);
    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[0].ctx.trackId == seq.getTrack(1).getId());
}

TEST_CASE("after snapshot swap, already-started note is not retriggered", "[engine][processor]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addNote(seq.getNumTracks() - 1, {60, 100, 0, 1000});
    const auto snapA = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snapA, 0);
    proc.process(snapA, 0, 100, sink);

    const auto snapB = PlaybackSnapshot::build(seq);
    sink.log.clear();
    proc.resetCursors(snapB, 100);
    proc.process(snapB, 100, 200, sink);

    for (const auto& c : sink.log)
        CHECK(c.kind != LoggedCall::NoteOn);
}

TEST_CASE("deleted note still gets note-off via stored ctx", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.setChannel(5);
    t.addNote({60, 100, 0, 200});
    seq.addTrack(t);
    const auto snapA = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snapA, 0);
    proc.process(snapA, 0, 100, sink);

    seq.removeNote(0, 0);
    const auto snapB = PlaybackSnapshot::build(seq);
    sink.log.clear();
    proc.resetCursors(snapB, 100);
    proc.process(snapB, 100, 300, sink);

    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::NoteOff);
    CHECK(sink.log[0].ctx.channel == 5);
}

TEST_CASE("seek forward repositions cursor", "[engine][processor]")
{
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 100, 100});
    t.addNote({62, 100, 1000, 200});
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 500);
    proc.process(snap, 500, 1100, sink);

    REQUIRE(sink.log.size() == 1);
    CHECK(sink.log[0].kind == LoggedCall::NoteOn);
    CHECK(sink.log[0].note.noteNumber == 62);
}

TEST_CASE("loop boundary cuts sounding notes and restarts from loopStart", "[engine][processor]")
{
    const int loopStart = 0;
    const int loopEnd = 480;
    MidiSequence seq;
    MidiTrack t;
    t.addNote({60, 100, 240, 480});
    t.addNote({62, 100, 0, 100});
    seq.addTrack(t);
    const auto snap = PlaybackSnapshot::build(seq);

    PlaybackProcessor proc;
    MockListener sink;
    proc.resetCursors(snap, 0);
    proc.process(snap, 0, 300, sink);

    sink.log.clear();
    proc.process(snap, 300, loopEnd, sink);
    proc.sendAllNoteOffs(sink);
    proc.resetCursors(snap, loopStart);
    proc.process(snap, loopStart, 10, sink);

    bool sawOff60 = false;
    bool sawOn62Again = false;
    for (const auto& c : sink.log)
    {
        if (c.kind == LoggedCall::NoteOff && c.note.noteNumber == 60)
            sawOff60 = true;
        if (c.kind == LoggedCall::NoteOn && c.note.noteNumber == 62)
            sawOn62Again = true;
    }
    CHECK(sawOff60);
    CHECK(sawOn62Again);
}
