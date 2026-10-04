#pragma once

#include "model/MidiEvent.h"
#include "model/MidiTrack.h"
#include <array>
#include <utility>

namespace ProjectXmlNames
{
inline constexpr int currentVersion = 1;

inline constexpr const char* projectTag = "CalliopeProject";
inline constexpr const char* versionAttr = "version";

inline constexpr const char* timelineTag = "Timeline";
inline constexpr const char* ppqAttr = "ppq";
inline constexpr const char* tempoTag = "Tempo";
inline constexpr const char* bpmAttr = "bpm";
inline constexpr const char* timeSignatureTag = "TimeSignature";
inline constexpr const char* numeratorAttr = "numerator";
inline constexpr const char* denominatorAttr = "denominator";

inline constexpr const char* keySignaturesTag = "KeySignatures";
inline constexpr const char* keySignatureTag = "KeySignature";
inline constexpr const char* sharpsOrFlatsAttr = "sharpsOrFlats";
inline constexpr const char* minorAttr = "minor";

inline constexpr const char* chordsTag = "Chords";
inline constexpr const char* chordTag = "Chord";
inline constexpr const char* rootAttr = "root";
inline constexpr const char* typeAttr = "type";
inline constexpr const char* bassRootAttr = "bassRoot";
inline constexpr const char* bassTypeAttr = "bassType";

inline constexpr const char* tracksTag = "Tracks";
inline constexpr const char* trackTag = "Track";
inline constexpr const char* idAttr = "id";
inline constexpr const char* nameAttr = "name";
inline constexpr const char* channelAttr = "channel";
inline constexpr const char* mutedAttr = "muted";
inline constexpr const char* soloAttr = "solo";
inline constexpr const char* outputAttr = "output";
inline constexpr const char* routeTargetAttr = "routeTarget";

inline constexpr const char* pluginTag = "Plugin";
inline constexpr const char* stateTag = "State";

inline constexpr const char* notesTag = "Notes";
inline constexpr const char* noteTag = "Note";
inline constexpr const char* tickAttr = "tick";
inline constexpr const char* durationAttr = "duration";
inline constexpr const char* keyAttr = "key";
inline constexpr const char* velocityAttr = "velocity";

inline constexpr const char* eventsTag = "Events";
inline constexpr const char* eventTag = "Event";
inline constexpr const char* data1Attr = "data1";
inline constexpr const char* data2Attr = "data2";

inline constexpr std::array<std::pair<MidiTrack::OutputDestination, const char*>, 3> outputs{{
    {MidiTrack::OutputDestination::MidiDevice, "midiDevice"},
    {MidiTrack::OutputDestination::Plugin, "plugin"},
    {MidiTrack::OutputDestination::None, "none"},
}};

inline constexpr std::array<std::pair<MidiEvent::Type, const char*>, 5> eventTypes{{
    {MidiEvent::Type::ControlChange, "controlChange"},
    {MidiEvent::Type::ProgramChange, "programChange"},
    {MidiEvent::Type::PitchBend, "pitchBend"},
    {MidiEvent::Type::ChannelPressure, "channelPressure"},
    {MidiEvent::Type::KeyPressure, "keyPressure"},
}};
} // namespace ProjectXmlNames
