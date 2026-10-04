#include "io/ProjectXml.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <juce_core/juce_core.h>
#include <optional>
#include <string>

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
