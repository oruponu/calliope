#include "io/ProjectXml.h"
#include "support/ChordTestHelpers.h"
#include "support/KeySignatureStringMaker.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiFileTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/TempoStringMaker.h"
#include "support/TimeSignatureStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <initializer_list>
#include <juce_core/juce_core.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::optional<SequenceContents> roundTrip(const MidiSequence& sequence,
                                          const ProjectXml::PluginStateSource& stateSource = {})
{
    const auto parsed = juce::parseXML(ProjectXml::write(sequence, stateSource)->toString());
    if (parsed == nullptr)
        return std::nullopt;
    return ProjectXml::read(*parsed);
}

std::string singleLineXml(const juce::XmlElement& element)
{
    return element.toString(juce::XmlElement::TextFormat().singleLine().withoutHeader()).toStdString();
}

std::string pluginDescriptionXml(const juce::String& name)
{
    juce::XmlElement element("PLUGIN");
    element.setAttribute("name", name);
    element.setAttribute("format", "VST3");
    return singleLineXml(element);
}

std::shared_ptr<const PluginAssignment> assignment(const std::string& description, std::vector<std::byte> state)
{
    return std::make_shared<const PluginAssignment>(PluginAssignment{description, std::move(state)});
}

std::vector<std::byte> bytes(std::initializer_list<int> values)
{
    std::vector<std::byte> result;
    for (int value : values)
        result.push_back(static_cast<std::byte>(value));
    return result;
}
} // namespace

TEST_CASE("the timeline, key signatures and chords come back unchanged", "[io][projectxml]")
{
    SequenceContents contents;
    contents.timeline.setTicksPerQuarterNote(960);
    contents.timeline.setTempoChanges({{0, 120.0}, {1920, 87.5}, {3840, 1.0 / 3.0}});
    contents.timeline.setTimeSignatureChanges({{0, 4, 4}, {3840, 7, 8}});
    contents.keySignatureChanges = {{0, -3, false}, {1920, 4, true}};
    contents.chordChanges = {chordtest::chord(0, "C"), chordtest::chord(960, "N.C."), {1920, 0x23, 5, 0x15, 200}};
    MidiSequence seq;
    seq.replaceContents(std::move(contents));

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTicksPerQuarterNote() == 960);
    CHECK(loaded->timeline.getTempoChanges() == seq.getTimeline().getTempoChanges());
    CHECK(loaded->timeline.getTimeSignatureChanges() == seq.getTimeline().getTimeSignatureChanges());
    CHECK(loaded->keySignatureChanges == seq.getKeySignatureChanges());
    CHECK(loaded->chordChanges == seq.getChordChanges());
}

TEST_CASE("the timeline lists its tempos before its time signatures", "[io][projectxml]")
{
    SequenceContents contents;
    contents.timeline.setTempoChanges({{0, 120.0}, {1920, 90.0}});
    contents.timeline.setTimeSignatureChanges({{0, 4, 4}, {3840, 3, 4}});
    MidiSequence seq;
    seq.replaceContents(std::move(contents));

    const auto written = ProjectXml::write(seq, {});
    const auto* timeline = written->getChildByName("Timeline");

    REQUIRE(timeline != nullptr);
    std::vector<std::string> tags;
    for (const auto* child : timeline->getChildIterator())
        tags.push_back(child->getTagName().toStdString());
    CHECK(tags == std::vector<std::string>{"Tempo", "Tempo", "TimeSignature", "TimeSignature"});
}

TEST_CASE("track settings come back unchanged", "[io][projectxml]")
{
    MidiSequence seq;
    MidiTrack drums;
    drums.setName("ドラム & <Kit> \"1\"");
    drums.setChannel(10);
    drums.setMuted(true);
    drums.setOutputDestination(MidiTrack::OutputDestination::None);
    seq.addTrack(drums);
    MidiTrack lead;
    lead.setName("Lead");
    lead.setChannel(16);
    lead.setSolo(true);
    seq.addTrack(lead);

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getName() == "ドラム & <Kit> \"1\"");
    CHECK(loaded->tracks[0].getChannel() == 10);
    CHECK(loaded->tracks[0].isMuted());
    CHECK_FALSE(loaded->tracks[0].isSolo());
    CHECK(loaded->tracks[0].getOutputDestination() == MidiTrack::OutputDestination::None);
    CHECK(loaded->tracks[1].getName() == "Lead");
    CHECK(loaded->tracks[1].getChannel() == 16);
    CHECK_FALSE(loaded->tracks[1].isMuted());
    CHECK(loaded->tracks[1].isSolo());
    CHECK(loaded->tracks[1].getOutputDestination() == MidiTrack::OutputDestination::MidiDevice);
}

TEST_CASE("notes and events come back in their stored order", "[io][projectxml]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({64, 1, 960, 0});
    track.addNote({0, 127, 0, 480});
    track.addNote({127, 0, 2147483646, 1});
    track.addEvent({MidiEvent::Type::PitchBend, 480, 16383, 0});
    track.addEvent({MidiEvent::Type::ControlChange, 0, 7, 100});
    track.addEvent({MidiEvent::Type::ProgramChange, 0, 41, 0});
    track.addEvent({MidiEvent::Type::ChannelPressure, 240, 64, 0});
    track.addEvent({MidiEvent::Type::KeyPressure, 360, 60, 33});
    seq.addTrack(track);

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() == track.getNotes());
    CHECK(loaded->tracks[0].getEvents() == track.getEvents());
}

TEST_CASE("route targets point to the same track after ids are reissued", "[io][projectxml]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackName(0, "Synth");
    seq.addTrack();
    seq.setTrackName(1, "Plain");
    seq.addTrack();
    seq.setTrackName(2, "Follower");
    seq.setTrackRouteTarget(2, seq.getTrack(0).getId());

    auto loaded = roundTrip(seq);
    REQUIRE(loaded);
    MidiSequence reopened;
    for (int i = 0; i < 4; ++i)
        reopened.addTrack();
    reopened.replaceContents(std::move(*loaded));

    const auto target = reopened.getTrack(2).getRouteTarget();
    REQUIRE(target);
    CHECK(reopened.getTrack(reopened.indexOf(*target)).getName() == "Synth");
    CHECK_FALSE(reopened.getTrack(1).getRouteTarget());
}

TEST_CASE("plugin states come back byte for byte and single-line descriptions unchanged", "[io][projectxml]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackPluginAssignment(
        0, assignment(pluginDescriptionXml("Synth & \"Co\""), bytes({0x00, 0xFF, 0x3C, 0x0A, 0x80})));
    seq.setTrackOutputDestination(0, MidiTrack::OutputDestination::Plugin);

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    const auto& restored = loaded->tracks[0].getPluginAssignment();
    REQUIRE(restored);
    CHECK(restored->descriptionXml == pluginDescriptionXml("Synth & \"Co\""));
    CHECK(restored->state == bytes({0x00, 0xFF, 0x3C, 0x0A, 0x80}));
    CHECK(loaded->tracks[0].getOutputDestination() == MidiTrack::OutputDestination::Plugin);
}

TEST_CASE("an assignment with an empty state comes back empty", "[io][projectxml]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackPluginAssignment(0, assignment(pluginDescriptionXml("Synth"), {}));

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks[0].getPluginAssignment());
    CHECK(loaded->tracks[0].getPluginAssignment()->state.empty());
}

TEST_CASE("the live state from the source replaces the stored state", "[io][projectxml]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackPluginAssignment(0, assignment(pluginDescriptionXml("Live"), bytes({9})));
    seq.addTrack();
    seq.setTrackPluginAssignment(1, assignment(pluginDescriptionXml("Missing"), bytes({7, 7})));
    const TrackId live = seq.getTrack(0).getId();

    const auto loaded = roundTrip(seq,
                                  [live](TrackId id) -> std::optional<std::vector<std::byte>>
                                  {
                                      if (id == live)
                                          return bytes({1, 2, 3});
                                      return std::nullopt;
                                  });

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getPluginAssignment()->state == bytes({1, 2, 3}));
    CHECK(loaded->tracks[1].getPluginAssignment()->state == bytes({7, 7}));
}

TEST_CASE("a single-line description that is not a plugin description comes back unchanged", "[io][projectxml]")
{
    juce::XmlElement element("SOMETHING");
    element.setAttribute("value", 1);
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackPluginAssignment(0, assignment(singleLineXml(element), {}));

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks[0].getPluginAssignment());
    CHECK(loaded->tracks[0].getPluginAssignment()->descriptionXml == singleLineXml(element));
}

TEST_CASE("a description written in another layout comes back in single-line form", "[io][projectxml]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackPluginAssignment(0, assignment("<PLUGIN  name=\"Synth\"\n  format=\"VST3\" />", {}));

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks[0].getPluginAssignment());
    CHECK(loaded->tracks[0].getPluginAssignment()->descriptionXml == pluginDescriptionXml("Synth"));
}

TEST_CASE("an empty song comes back with no tracks", "[io][projectxml]")
{
    MidiSequence seq;
    seq.replaceContents({});

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    CHECK(loaded->tracks.empty());
    CHECK(loaded->keySignatureChanges.empty());
    CHECK(loaded->chordChanges.empty());
}

TEST_CASE("a MIDI import with a mid-bar time signature saves and reopens", "[io][projectxml]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(midifiletest::at(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0));
    conductor.addEvent(midifiletest::at(juce::MidiMessage::timeSignatureMetaEvent(3, 4), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    auto imported = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));
    REQUIRE(imported);
    MidiSequence seq;
    seq.replaceContents(std::move(*imported));

    const auto loaded = roundTrip(seq);

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() == seq.getTimeline().getTimeSignatureChanges());
}
