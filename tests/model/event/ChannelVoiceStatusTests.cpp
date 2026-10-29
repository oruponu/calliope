#include "model/event/ChannelVoiceStatus.h"
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_basics/juce_audio_basics.h>

TEST_CASE("only status bytes 0x80 to 0xEF are channel voice messages", "[model][event]")
{
    CHECK(isChannelVoiceStatus(0x80));
    CHECK(isChannelVoiceStatus(0x9F));
    CHECK(isChannelVoiceStatus(0xA0));
    CHECK(isChannelVoiceStatus(0xB5));
    CHECK(isChannelVoiceStatus(0xC0));
    CHECK(isChannelVoiceStatus(0xD3));
    CHECK(isChannelVoiceStatus(0xEF));
    CHECK_FALSE(isChannelVoiceStatus(0x7F));
    CHECK_FALSE(isChannelVoiceStatus(0xF0));
    CHECK_FALSE(isChannelVoiceStatus(0xF7));
    CHECK_FALSE(isChannelVoiceStatus(0xF8));
    CHECK_FALSE(isChannelVoiceStatus(0xFE));
    CHECK_FALSE(isChannelVoiceStatus(0xFF));
}

TEST_CASE("the status byte of every JUCE channel voice message passes", "[model][event]")
{
    CHECK(isChannelVoiceStatus(juce::MidiMessage::noteOn(1, 60, 0.5f).getRawData()[0]));
    CHECK(isChannelVoiceStatus(juce::MidiMessage::noteOff(16, 60).getRawData()[0]));
    CHECK(isChannelVoiceStatus(juce::MidiMessage::aftertouchChange(1, 60, 10).getRawData()[0]));
    CHECK(isChannelVoiceStatus(juce::MidiMessage::controllerEvent(1, 7, 100).getRawData()[0]));
    CHECK(isChannelVoiceStatus(juce::MidiMessage::programChange(1, 5).getRawData()[0]));
    CHECK(isChannelVoiceStatus(juce::MidiMessage::channelPressureChange(1, 10).getRawData()[0]));
    CHECK(isChannelVoiceStatus(juce::MidiMessage::pitchWheel(16, 8192).getRawData()[0]));
    CHECK_FALSE(isChannelVoiceStatus(juce::MidiMessage::midiClock().getRawData()[0]));
    CHECK_FALSE(isChannelVoiceStatus(juce::MidiMessage::createSysExMessage("\x41", 1).getRawData()[0]));
}
