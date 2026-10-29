#include "io/project/ProjectXml.h"
#include "support/MetaEventStringMaker.h"
#include "support/MidiEventTestHelpers.h"
#include "support/SysExEventStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <juce_core/juce_core.h>
#include <optional>
#include <string>
#include <vector>

namespace
{
const std::string validProject =
    R"(<CalliopeProject version="1">)"
    R"(<Timeline ppq="480"><Tempo tick="0" bpm="120"/><TimeSignature tick="0" numerator="4" denominator="4"/></Timeline>)"
    R"(<KeySignatures><KeySignature tick="0" sharpsOrFlats="0" minor="0"/></KeySignatures>)"
    R"(<Chords><Chord tick="0" root="1" type="0" bassRoot="127" bassType="127"/></Chords>)"
    R"(<Tracks>)"
    R"(<Track id="1" name="A" channel="1" muted="0" solo="0" output="plugin">)"
    R"(<Plugin><PLUGIN name="Synth"/><State>AAEC</State></Plugin>)"
    R"(<Notes><Note tick="0" duration="480" key="60" velocity="100"/></Notes>)"
    R"(<Events><Event type="controlChange" tick="0" data1="7" data2="100"/></Events>)"
    R"(</Track>)"
    R"(<Track id="2" name="B" channel="2" muted="1" solo="1" output="plugin" routeTarget="1"><Notes/><Events/></Track>)"
    R"(</Tracks>)"
    R"(</CalliopeProject>)";

std::optional<SequenceContents> readText(const std::string& text)
{
    const auto parsed = juce::parseXML(juce::String::fromUTF8(text.data(), static_cast<int>(text.size())));
    if (parsed == nullptr)
        return std::nullopt;
    return ProjectXml::read(*parsed);
}

std::string replaced(std::string text, const std::string& from, const std::string& to)
{
    const auto at = text.find(from);
    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), to);
    return text;
}

std::string replacedAll(std::string text, const std::string& from, const std::string& to)
{
    REQUIRE(text.find(from) != std::string::npos);
    for (auto at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
        text.replace(at, from.size(), to);
    return text;
}
} // namespace

TEST_CASE("the reference project is read", "[io][projectxml]")
{
    const auto loaded = readText(validProject);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    REQUIRE(loaded->tracks[0].getPluginAssignment());
    CHECK(loaded->tracks[0].getPluginAssignment()->descriptionXml == R"(<PLUGIN name="Synth"/>)");
    CHECK(loaded->tracks[0].getPluginAssignment()->state.size() == 3);
    CHECK(loaded->tracks[0].getId() == TrackId{1});
    CHECK(loaded->tracks[1].getRouteTarget() == TrackId{1});
    CHECK(loaded->tracks[1].isMuted());
    CHECK(loaded->tracks[1].isSolo());
}

TEST_CASE("a track without SysExEvents has no sysex", "[io][projectxml][sysex]")
{
    const auto loaded = readText(validProject);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getSysExEvents().empty());
    CHECK(loaded->tracks[1].getSysExEvents().empty());
}

TEST_CASE("sysex in a project is read", "[io][projectxml][sysex]")
{
    const auto loaded = readText(replaced(
        validProject, "</Events>",
        R"(</Events><SysExEvents><SysEx tick="0" data="41 10 42"/><SysEx tick="0" data="7F"/></SysExEvents>)"));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, {0x41, 0x10, 0x42}}, {0, {0x7F}}});
}

TEST_CASE("malformed sysex is rejected", "[io][projectxml][sysex]")
{
    const auto sysEx =
        GENERATE(as<std::string>{}, R"(<SysEx tick="0" data=""/>)", R"(<SysEx tick="0" data="41  10"/>)",
                 R"(<SysEx tick="0" data="7f"/>)", R"(<SysEx tick="0" data="4G"/>)", R"(<SysEx tick="0" data="80"/>)",
                 R"(<SysEx tick="-1" data="41"/>)", R"(<SysEx data="41"/>)", R"(<SysEx tick="0"/>)",
                 R"(<SysEx tick="480" data="41"/><SysEx tick="0" data="42"/>)");
    CAPTURE(sysEx);
    CHECK_FALSE(readText(replaced(validProject, "</Events>", "</Events><SysExEvents>" + sysEx + "</SysExEvents>")));
}

TEST_CASE("a project without MetaEvents has no meta events", "[io][projectxml][meta]")
{
    const auto loaded = readText(validProject);

    REQUIRE(loaded);
    CHECK(loaded->metaEvents.empty());
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getMetaEvents().empty());
    CHECK(loaded->tracks[1].getMetaEvents().empty());
}

TEST_CASE("meta events in a project are read", "[io][projectxml][meta]")
{
    auto project = replaced(
        validProject, "</Events>",
        R"(</Events><MetaEvents><Meta tick="0" type="5" data="82 A0"/><Meta tick="0" type="0" data=""/></MetaEvents>)");
    project =
        replaced(project, "</Chords>", R"(</Chords><MetaEvents><Meta tick="0" type="3" data="53"/></MetaEvents>)");

    const auto loaded = readText(project);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getMetaEvents() == std::vector<MetaEvent>{{0, 0x05, {0x82, 0xA0}}, {0, 0x00, {}}});
    CHECK(loaded->metaEvents == std::vector<MetaEvent>{{0, 0x03, {0x53}}});
}

TEST_CASE("malformed meta events are rejected", "[io][projectxml][meta]")
{
    const auto meta = GENERATE(as<std::string>{}, R"(<Meta tick="0" type="5" data="8"/>)",
                               R"(<Meta tick="0" type="5" data="82  A0"/>)", R"(<Meta tick="0" type="5" data="a0"/>)",
                               R"(<Meta tick="0" type="256" data=""/>)", R"(<Meta tick="0" type="-1" data=""/>)",
                               R"(<Meta tick="0" type="128" data=""/>)", R"(<Meta tick="0" type="47" data=""/>)",
                               R"(<Meta tick="0" type="3" data="53"/>)", R"(<Meta tick="-1" type="5" data=""/>)",
                               R"(<Meta tick="0" data=""/>)", R"(<Meta tick="0" type="5"/>)",
                               R"(<Meta tick="480" type="5" data=""/><Meta tick="0" type="5" data=""/>)");
    CAPTURE(meta);
    CHECK_FALSE(readText(replaced(validProject, "</Events>", "</Events><MetaEvents>" + meta + "</MetaEvents>")));
}

TEST_CASE("a song track name is accepted but a song end of track is not", "[io][projectxml][meta]")
{
    CHECK(readText(replaced(validProject, "</Chords>",
                            R"(</Chords><MetaEvents><Meta tick="0" type="3" data="53"/></MetaEvents>)")));
    CHECK_FALSE(readText(replaced(validProject, "</Chords>",
                                  R"(</Chords><MetaEvents><Meta tick="0" type="47" data=""/></MetaEvents>)")));
}

TEST_CASE("a document that is not a Calliope project is rejected", "[io][projectxml]")
{
    CHECK_FALSE(readText(replacedAll(validProject, "CalliopeProject", "Project")));
}

TEST_CASE("only version 1 is accepted", "[io][projectxml]")
{
    const auto version =
        GENERATE(as<std::string>{}, R"(version="2")", R"(version="0")", R"(version="1.0")", R"(version="")", "");
    CAPTURE(version);
    CHECK_FALSE(readText(replaced(validProject, R"(version="1")", version)));
}

TEST_CASE("a missing section is rejected", "[io][projectxml]")
{
    const auto [from, to] = GENERATE(table<std::string, std::string>({{"Timeline", "Timing"},
                                                                      {"KeySignatures", "KeyMarks"},
                                                                      {"Chords", "Harmony"},
                                                                      {"Tracks", "Lanes"},
                                                                      {"Notes", "Nodes"},
                                                                      {"Events", "Happenings"}}));
    CAPTURE(from);
    CHECK_FALSE(readText(replacedAll(validProject, from, to)));
}

TEST_CASE("a missing required attribute is rejected", "[io][projectxml]")
{
    const auto attribute = GENERATE(
        as<std::string>{}, R"( ppq="480")", R"( bpm="120")", R"( numerator="4")", R"( sharpsOrFlats="0")",
        R"( minor="0")", R"( bassType="127")", R"( id="1")", R"( name="A")", R"( channel="1")", R"( muted="0")",
        R"( solo="0")", R"( output="plugin")", R"( duration="480")", R"( velocity="100")", R"( data2="100")");
    CAPTURE(attribute);
    CHECK_FALSE(readText(replaced(validProject, attribute, "")));
}

TEST_CASE("a number with anything but the number is rejected", "[io][projectxml]")
{
    const auto [from, to] =
        GENERATE(table<std::string, std::string>({{R"(ppq="480")", R"(ppq="480x")"},
                                                  {R"(bpm="120")", R"(bpm="120bpm")"},
                                                  {R"(bpm="120")", R"(bpm="inf")"},
                                                  {R"(bpm="120")", R"(bpm="nan")"},
                                                  {R"(key="60")", R"(key="60.0")"},
                                                  {R"(duration="480")", R"(duration="4294967296")"},
                                                  {R"(tick="0" duration)", R"(tick=" 0" duration)"}}));
    CAPTURE(to);
    CHECK_FALSE(readText(replaced(validProject, from, to)));
}

TEST_CASE("flags must be 0 or 1", "[io][projectxml]")
{
    const auto [from, to] = GENERATE(table<std::string, std::string>(
        {{R"(muted="0")", R"(muted="2")"}, {R"(solo="0")", R"(solo="true")"}, {R"(minor="0")", R"(minor="yes")"}}));
    CAPTURE(to);
    CHECK_FALSE(readText(replaced(validProject, from, to)));
}

TEST_CASE("unknown output and event names are rejected", "[io][projectxml]")
{
    const auto [from, to] = GENERATE(table<std::string, std::string>(
        {{R"(output="plugin")", R"(output="speaker")"}, {R"(type="controlChange")", R"(type="sysex")"}}));
    CAPTURE(to);
    CHECK_FALSE(readText(replaced(validProject, from, to)));
}

TEST_CASE("a plugin without a description is rejected", "[io][projectxml]")
{
    CHECK_FALSE(readText(replaced(validProject, R"(<PLUGIN name="Synth"/>)", "")));
}

TEST_CASE("a plugin state that is not Base64 is rejected", "[io][projectxml]")
{
    const auto state = GENERATE(as<std::string>{}, "@@@@", "AAE");
    CAPTURE(state);
    CHECK_FALSE(readText(replaced(validProject, "AAEC", state)));
}

TEST_CASE("track ids and route targets must be positive and ids unique", "[io][projectxml]")
{
    const auto [from, to] = GENERATE(table<std::string, std::string>({{R"(id="1")", R"(id="0")"},
                                                                      {R"(id="2")", R"(id="1")"},
                                                                      {R"(id="2")", R"(id="-2")"},
                                                                      {R"(routeTarget="1")", R"(routeTarget="0")"}}));
    CAPTURE(to);
    CHECK_FALSE(readText(replaced(validProject, from, to)));
}

TEST_CASE("a route target to a missing track is read as written", "[io][projectxml]")
{
    const auto loaded = readText(replaced(validProject, R"(routeTarget="1")", R"(routeTarget="99")"));

    REQUIRE(loaded);
    CHECK(loaded->tracks[1].getRouteTarget() == TrackId{99});
}

TEST_CASE("values the model does not accept are rejected", "[io][projectxml]")
{
    const auto [from, to] = GENERATE(table<std::string, std::string>({{R"(bpm="120")", R"(bpm="0")"},
                                                                      {R"(channel="1")", R"(channel="17")"},
                                                                      {R"(velocity="100")", R"(velocity="128")"}}));
    CAPTURE(to);
    CHECK_FALSE(readText(replaced(validProject, from, to)));
}

TEST_CASE("steps saved before ramps are read as points", "[io][projectxml]")
{
    const auto loaded = readText(replaced(
        validProject, R"(<Event type="controlChange" tick="0" data1="7" data2="100"/>)",
        R"(<Event type="controlChange" tick="0" data1="7" data2="100"/><Event type="controlChange" tick="960" data1="7" data2="50"/>)"));

    REQUIRE(loaded);
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ControlChange, 0, 7, 100, true},
                                                                  {MidiEvent::Type::ControlChange, 960, 7, 100, true},
                                                                  {MidiEvent::Type::ControlChange, 960, 7, 50, true}});
}

TEST_CASE("a ramp attribute is read", "[io][projectxml]")
{
    const auto loaded = readText(replaced(validProject, R"(data2="100")", R"(data2="100" ramp="1")"));

    REQUIRE(loaded);
    CHECK(loaded->tracks[0].getEvent(0).ramp);
}

TEST_CASE("a ramp attribute other than 0 or 1 is rejected", "[io][projectxml]")
{
    CHECK_FALSE(readText(replaced(validProject, R"(data2="100")", R"(data2="100" ramp="yes")")));
}

TEST_CASE("a ramp on a program change is rejected", "[io][projectxml]")
{
    CHECK_FALSE(readText(replaced(validProject, R"(type="controlChange" tick="0" data1="7" data2="100")",
                                  R"(type="programChange" tick="0" data1="7" data2="0" ramp="1")")));
}
