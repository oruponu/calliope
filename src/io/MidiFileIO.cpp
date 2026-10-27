#include "io/MidiFileIO.h"
#include "edit/ChordTrackEdits.h"
#include "edit/KeySignatureEdits.h"
#include "edit/TempoEdits.h"
#include "edit/TimeSignatureEdits.h"
#include "io/MetaText.h"
#include "model/ControllerPointConversion.h"
#include "model/ControllerRamps.h"
#include "model/KeySignatureChange.h"
#include "model/MetaEventRetention.h"
#include "model/SequenceContentsRules.h"
#include "model/TimeSignatureChange.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace
{
std::optional<TimeSignatureChange> readTimeSignature(const juce::MidiMessage& msg, int ppq)
{
    const auto* data = msg.getMetaEventData();
    if (msg.getMetaEventLength() < 2 || data[1] > 30)
        return std::nullopt;

    const TimeSignatureChange change{static_cast<int>(msg.getTimeStamp()), data[0], 1 << data[1]};
    if (!SequenceContentsRules::acceptsTimeSignature(change, ppq))
        return std::nullopt;
    return change;
}

std::optional<KeySignatureChange> readKeySignature(const juce::MidiMessage& msg)
{
    const auto* data = msg.getMetaEventData();
    if (msg.getMetaEventLength() < 2)
        return std::nullopt;

    const KeySignatureChange change{static_cast<int>(msg.getTimeStamp()), static_cast<std::int8_t>(data[0]),
                                    data[1] != 0};
    if (!SequenceContentsRules::acceptsKeySignature(change))
        return std::nullopt;
    return change;
}

std::optional<SysExEvent> readSysEx(const juce::MidiMessage& msg)
{
    if (!msg.isSysEx())
        return std::nullopt;

    const int dataSize = msg.getSysExDataSize();
    if (dataSize <= 0 || msg.getRawData()[msg.getRawDataSize() - 1] != 0xF7)
        return std::nullopt;

    const auto* data = msg.getSysExData();
    return SysExEvent{static_cast<int>(msg.getTimeStamp()), {data, data + dataSize}};
}

std::optional<MetaEvent> readMetaEvent(const juce::MidiMessage& msg, MetaEventRetention::Owner owner)
{
    if (!msg.isMetaEvent())
        return std::nullopt;

    std::vector<std::uint8_t> data;
    const int dataSize = msg.getMetaEventLength();
    if (const auto* bytes = msg.getMetaEventData(); bytes != nullptr && dataSize > 0)
        data.assign(bytes, bytes + dataSize);

    const int type = msg.getMetaEventType();
    if (!MetaEventRetention::isRetained(type, data, owner))
        return std::nullopt;

    return MetaEvent{static_cast<int>(msg.getTimeStamp()), static_cast<std::uint8_t>(type), std::move(data)};
}

juce::MidiMessage metaMessage(const MetaEvent& event)
{
    std::vector<std::uint8_t> bytes{0xFF, event.type};
    std::vector<std::uint8_t> lengthBytes;
    auto length = event.data.size();
    do
    {
        lengthBytes.push_back(static_cast<std::uint8_t>(length & 0x7F));
        length >>= 7;
    } while (length != 0);

    for (std::size_t i = lengthBytes.size(); i-- > 0;)
        bytes.push_back(static_cast<std::uint8_t>(lengthBytes[i] | (i == 0 ? 0x00 : 0x80)));
    bytes.insert(bytes.end(), event.data.begin(), event.data.end());

    return juce::MidiMessage(bytes.data(), static_cast<int>(bytes.size()), event.tick);
}

} // anonymous namespace

bool MidiFileIO::save(const MidiSequence& sequence, const juce::File& file)
{
    juce::MidiFile midiFile;
    int ppq = sequence.getTimeline().getTicksPerQuarterNote();
    midiFile.setTicksPerQuarterNote(ppq);

    juce::MidiMessageSequence tempoTrack;

    for (const auto& event : sequence.getMetaEvents())
        tempoTrack.addEvent(metaMessage(event));

    for (const auto& tc : sequence.getTimeline().getTempoChanges())
    {
        int microsecondsPerBeat = static_cast<int>(60000000.0 / tc.bpm);
        auto tempoEvent = juce::MidiMessage::tempoMetaEvent(microsecondsPerBeat);
        tempoEvent.setTimeStamp(tc.tick);
        tempoTrack.addEvent(tempoEvent);
    }

    for (const auto& ts : sequence.getTimeline().getTimeSignatureChanges())
    {
        auto tsEvent = juce::MidiMessage::timeSignatureMetaEvent(ts.numerator, ts.denominator);
        tsEvent.setTimeStamp(ts.tick);
        tempoTrack.addEvent(tsEvent);
    }

    for (const auto& ks : sequence.getKeySignatureChanges())
    {
        auto ksEvent = juce::MidiMessage::keySignatureMetaEvent(ks.sharpsOrFlats, ks.isMinor);
        ksEvent.setTimeStamp(ks.tick);
        tempoTrack.addEvent(ksEvent);
    }

    for (const auto& cc : sequence.getChordChanges())
    {
        // XF Chord: FF 7F 07 43 7B 01 cr ct bn bt
        uint8_t raw[] = {0xFF,
                         0x7F,
                         0x07,
                         0x43,
                         0x7B,
                         0x01,
                         static_cast<uint8_t>(cc.chordRoot),
                         static_cast<uint8_t>(cc.chordType),
                         static_cast<uint8_t>(cc.bassRoot),
                         static_cast<uint8_t>(cc.bassType)};
        auto msg = juce::MidiMessage(raw, static_cast<int>(sizeof(raw)));
        msg.setTimeStamp(cc.tick);
        tempoTrack.addEvent(msg);
    }

    tempoTrack.sort();

    auto endOfTempoTrack = juce::MidiMessage::endOfTrack();
    endOfTempoTrack.setTimeStamp(tempoTrack.getEndTime());
    tempoTrack.addEvent(endOfTempoTrack);
    midiFile.addTrack(tempoTrack);

    for (int t = 0; t < sequence.getNumTracks(); ++t)
    {
        const auto& track = sequence.getTrack(t);
        juce::MidiMessageSequence msgSeq;
        int lastTick = 0;
        const int trackChannel = track.getChannel();

        if (!track.getName().empty())
        {
            auto nameEvent = juce::MidiMessage::textMetaEvent(3, track.getName());
            nameEvent.setTimeStamp(0);
            msgSeq.addEvent(nameEvent);
        }

        // Lets a track without channel events keep its channel.
        auto channelPrefix = juce::MidiMessage::midiChannelMetaEvent(trackChannel);
        channelPrefix.setTimeStamp(0);
        msgSeq.addEvent(channelPrefix);

        for (const auto& event : track.getMetaEvents())
        {
            msgSeq.addEvent(metaMessage(event));

            if (event.tick > lastTick)
                lastTick = event.tick;
        }

        for (const auto& sysEx : track.getSysExEvents())
        {
            auto msg = juce::MidiMessage::createSysExMessage(sysEx.data.data(), static_cast<int>(sysEx.data.size()));
            msg.setTimeStamp(sysEx.tick);
            msgSeq.addEvent(msg);

            if (sysEx.tick > lastTick)
                lastTick = sysEx.tick;
        }

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);

            auto noteOn =
                juce::MidiMessage::noteOn(trackChannel, note.noteNumber, static_cast<juce::uint8>(note.velocity));
            noteOn.setTimeStamp(note.startTick);
            msgSeq.addEvent(noteOn);

            auto noteOff = juce::MidiMessage::noteOff(trackChannel, note.noteNumber);
            noteOff.setTimeStamp(note.endTick());
            msgSeq.addEvent(noteOff);

            if (note.endTick() > lastTick)
                lastTick = note.endTick();
        }

        for (const auto& event : expandControllerRamps(track.getEvents()))
        {
            juce::MidiMessage msg;

            switch (event.type)
            {
            case MidiEvent::Type::ControlChange:
                msg = juce::MidiMessage::controllerEvent(trackChannel, event.data1, event.data2);
                break;
            case MidiEvent::Type::ProgramChange:
                msg = juce::MidiMessage::programChange(trackChannel, event.data1);
                break;
            case MidiEvent::Type::PitchBend:
                msg = juce::MidiMessage::pitchWheel(trackChannel, event.data1);
                break;
            case MidiEvent::Type::ChannelPressure:
                msg = juce::MidiMessage::channelPressureChange(trackChannel, event.data1);
                break;
            case MidiEvent::Type::KeyPressure:
                msg = juce::MidiMessage::aftertouchChange(trackChannel, event.data1, event.data2);
                break;
            }

            msg.setTimeStamp(event.tick);
            msgSeq.addEvent(msg);

            if (event.tick > lastTick)
                lastTick = event.tick;
        }

        msgSeq.sort();

        auto endOfTrack = juce::MidiMessage::endOfTrack();
        endOfTrack.setTimeStamp(lastTick);
        msgSeq.addEvent(endOfTrack);

        midiFile.addTrack(msgSeq);
    }

    juce::TemporaryFile temp(file);
    {
        juce::FileOutputStream stream(temp.getFile());
        if (!stream.openedOk() || !midiFile.writeTo(stream, 1))
            return false;

        // FileOutputStream buffers writes, so a failure may only surface in flush(), which reports it via getStatus().
        stream.flush();
        if (stream.getStatus().failed())
            return false;
    }

    return temp.overwriteTargetFileWithTemporary();
}

std::optional<SequenceContents> MidiFileIO::load(const juce::File& file)
{
    juce::MemoryBlock fileData;
    if (!file.loadFileAsData(fileData))
        return std::nullopt;

    auto* data = static_cast<const uint8_t*>(fileData.getData());
    size_t size = fileData.getSize();

    // Drop non-standard chunks other than MThd and MTrk (e.g. XFIH in YAMAHA XG files)
    juce::MemoryBlock filteredData;
    {
        size_t pos = 0;
        while (pos + 8 <= size)
        {
            uint32_t chunkSize = (data[pos + 4] << 24) | (data[pos + 5] << 16) | (data[pos + 6] << 8) | data[pos + 7];
            size_t totalSize = 8 + chunkSize;
            if (pos + totalSize > size)
                break;

            if (memcmp(data + pos, "MThd", 4) == 0 || memcmp(data + pos, "MTrk", 4) == 0)
                filteredData.append(data + pos, totalSize);

            pos += totalSize;
        }
    }

    int format = 0;
    if (filteredData.getSize() >= 10)
    {
        auto* hdr = static_cast<const uint8_t*>(filteredData.getData());
        format = (hdr[8] << 8) | hdr[9];
    }

    juce::MemoryInputStream stream(filteredData.getData(), filteredData.getSize(), false);

    juce::MidiFile midiFile;
    if (!midiFile.readFrom(stream))
        return std::nullopt;

    SequenceContents contents;

    int ppq = midiFile.getTimeFormat();
    if (ppq <= 0)
        ppq = TimelineMap::defaultTicksPerQuarterNote;
    contents.timeline.setTicksPerQuarterNote(ppq);

    auto tempoChanges = contents.timeline.getTempoChanges();
    auto timeSignatureChanges = contents.timeline.getTimeSignatureChanges();
    auto keySignatureChanges = contents.keySignatureChanges;
    auto chordChanges = contents.chordChanges;

    for (int t = 0; t < midiFile.getNumTracks(); ++t)
    {
        const auto* msgSeq = midiFile.getTrack(t);
        for (int i = 0; i < msgSeq->getNumEvents(); ++i)
        {
            const auto& msg = msgSeq->getEventPointer(i)->message;
            if (msg.isTempoMetaEvent())
            {
                const double secondsPerQuarter = msg.getTempoSecondsPerQuarterNote();
                if (secondsPerQuarter <= 0.0)
                    continue;
                double bpm = 60000000.0 / secondsPerQuarter / 1000000.0;
                int tick = static_cast<int>(msg.getTimeStamp());
                TempoEdits::add(tempoChanges, tick, bpm);
            }
            else if (msg.isTimeSignatureMetaEvent())
            {
                if (const auto ts = readTimeSignature(msg, ppq))
                    TimeSignatureEdits::add(timeSignatureChanges, ts->tick, ts->numerator, ts->denominator, ppq);
            }
            else if (msg.isKeySignatureMetaEvent())
            {
                if (const auto ks = readKeySignature(msg))
                    KeySignatureEdits::add(keySignatureChanges, ks->tick, ks->sharpsOrFlats, ks->isMinor);
            }
            else if (msg.getMetaEventType() == 0x7F)
            {
                auto* metaData = msg.getMetaEventData();
                int len = msg.getMetaEventLength();
                // XF Chord: 43 7B 01 cr ct bn bt
                if (metaData != nullptr && len >= 7 && metaData[0] == 0x43 && metaData[1] == 0x7B &&
                    metaData[2] == 0x01)
                {
                    int tick = static_cast<int>(msg.getTimeStamp());
                    ChordTrackEdits::add(chordChanges, tick, metaData[3], metaData[4], metaData[5], metaData[6]);
                }
            }
        }
    }

    contents.timeline.setTempoChanges(std::move(tempoChanges));
    contents.timeline.setTimeSignatureChanges(std::move(timeSignatureChanges));
    contents.keySignatureChanges = std::move(keySignatureChanges);
    contents.chordChanges = std::move(chordChanges);

    if (format == 0)
    {
        // Format 0: SysEx goes to its own track, then one track per channel
        std::set<int> usedChannels;
        std::vector<SysExEvent> sysExEvents;
        std::vector<MetaEvent> metaEvents;
        for (int t = 0; t < midiFile.getNumTracks(); ++t)
        {
            const auto* msgSeq = midiFile.getTrack(t);
            for (int i = 0; i < msgSeq->getNumEvents(); ++i)
            {
                const auto& msg = msgSeq->getEventPointer(i)->message;
                if (msg.isNoteOn() || msg.isController() || msg.isProgramChange() || msg.isPitchWheel() ||
                    msg.isChannelPressure() || msg.isAftertouch())
                {
                    usedChannels.insert(msg.getChannel());
                }
                else if (const auto sysEx = readSysEx(msg))
                {
                    sysExEvents.push_back(*sysEx);
                }

                if (const auto metaEvent = readMetaEvent(msg, MetaEventRetention::Owner::Song))
                    metaEvents.push_back(*metaEvent);
            }
        }

        std::stable_sort(metaEvents.begin(), metaEvents.end(),
                         [](const MetaEvent& a, const MetaEvent& b) { return a.tick < b.tick; });
        contents.metaEvents.insert(contents.metaEvents.end(), metaEvents.begin(), metaEvents.end());

        if (!sysExEvents.empty())
        {
            auto& track = contents.tracks.emplace_back();
            track.setName("SysEx");
            track.setChannel(1);
            for (const auto& sysEx : sysExEvents)
                track.addSysEx(sysEx);
        }

        std::map<int, int> channelTrackIndex;
        for (int ch : usedChannels)
        {
            int index = static_cast<int>(contents.tracks.size());
            auto& track = contents.tracks.emplace_back();
            track.setName("Ch." + std::to_string(ch));
            track.setChannel(ch);
            channelTrackIndex[ch] = index;
        }

        for (int t = 0; t < midiFile.getNumTracks(); ++t)
        {
            const auto* msgSeq = midiFile.getTrack(t);
            juce::MidiMessageSequence sorted(*msgSeq);
            sorted.updateMatchedPairs();

            for (int i = 0; i < sorted.getNumEvents(); ++i)
            {
                const auto* event = sorted.getEventPointer(i);
                const auto& msg = event->message;

                if (msg.isNoteOn())
                {
                    auto& track = contents.tracks[static_cast<size_t>(channelTrackIndex[msg.getChannel()])];

                    int noteNumber = msg.getNoteNumber();
                    int velocity = msg.getVelocity();
                    int startTick = static_cast<int>(msg.getTimeStamp());
                    int endTick = startTick + ppq;

                    if (event->noteOffObject != nullptr)
                        endTick = static_cast<int>(event->noteOffObject->message.getTimeStamp());

                    track.addNote({noteNumber, velocity, startTick, endTick - startTick});
                }
                else if (msg.isController())
                {
                    auto& track = contents.tracks[static_cast<size_t>(channelTrackIndex[msg.getChannel()])];

                    track.addEvent({.type = MidiEvent::Type::ControlChange,
                                    .tick = static_cast<int>(msg.getTimeStamp()),
                                    .data1 = msg.getControllerNumber(),
                                    .data2 = msg.getControllerValue()});
                }
                else if (msg.isProgramChange())
                {
                    auto& track = contents.tracks[static_cast<size_t>(channelTrackIndex[msg.getChannel()])];

                    track.addEvent({.type = MidiEvent::Type::ProgramChange,
                                    .tick = static_cast<int>(msg.getTimeStamp()),
                                    .data1 = msg.getProgramChangeNumber()});
                }
                else if (msg.isPitchWheel())
                {
                    auto& track = contents.tracks[static_cast<size_t>(channelTrackIndex[msg.getChannel()])];

                    track.addEvent({.type = MidiEvent::Type::PitchBend,
                                    .tick = static_cast<int>(msg.getTimeStamp()),
                                    .data1 = msg.getPitchWheelValue()});
                }
                else if (msg.isChannelPressure())
                {
                    auto& track = contents.tracks[static_cast<size_t>(channelTrackIndex[msg.getChannel()])];

                    track.addEvent({.type = MidiEvent::Type::ChannelPressure,
                                    .tick = static_cast<int>(msg.getTimeStamp()),
                                    .data1 = msg.getChannelPressureValue()});
                }
                else if (msg.isAftertouch())
                {
                    auto& track = contents.tracks[static_cast<size_t>(channelTrackIndex[msg.getChannel()])];

                    track.addEvent({.type = MidiEvent::Type::KeyPressure,
                                    .tick = static_cast<int>(msg.getTimeStamp()),
                                    .data1 = msg.getNoteNumber(),
                                    .data2 = msg.getAfterTouchValue()});
                }
            }
        }
    }
    else
    {
        // Format 1 and later: use the MIDI file's tracks as-is
        for (int t = 0; t < midiFile.getNumTracks(); ++t)
        {
            const auto* msgSeq = midiFile.getTrack(t);
            juce::MidiMessageSequence sorted(*msgSeq);
            sorted.updateMatchedPairs();

            MidiTrack* track = nullptr;
            juce::String trackName;
            std::optional<int> prefixChannel;
            std::optional<int> firstChannel;

            const auto ensureTrack = [&]() -> MidiTrack&
            {
                if (!track)
                    track = &contents.tracks.emplace_back();
                return *track;
            };

            const auto trackForChannel = [&](int channel) -> MidiTrack&
            {
                auto& result = ensureTrack();
                if (!firstChannel)
                {
                    firstChannel = channel;
                    result.setChannel(channel);
                }
                return result;
            };

            const auto addMetaEvent = [&](const MetaEvent& event)
            {
                if (t == 0)
                    contents.metaEvents.push_back(event);
                else
                    ensureTrack().addMetaEvent(event);
            };

            const auto metaOwner = t == 0 ? MetaEventRetention::Owner::Song : MetaEventRetention::Owner::Track;

            for (int i = 0; i < sorted.getNumEvents(); ++i)
            {
                const auto* event = sorted.getEventPointer(i);
                const auto& msg = event->message;

                if (msg.isTrackNameEvent())
                {
                    trackName = MetaText::decode(msg.getMetaEventData(), msg.getMetaEventLength());
                    if (const auto metaEvent = readMetaEvent(msg, metaOwner))
                        addMetaEvent(*metaEvent);
                }
                else if (msg.isMidiChannelMetaEvent())
                {
                    const int channel = msg.getMidiChannelMetaEventChannel();
                    if (!prefixChannel && channel >= 1 && channel <= 16)
                        prefixChannel = channel;
                }
                else if (msg.isNoteOn())
                {
                    auto& channelTrack = trackForChannel(msg.getChannel());

                    int noteNumber = msg.getNoteNumber();
                    int velocity = msg.getVelocity();
                    int startTick = static_cast<int>(msg.getTimeStamp());
                    int endTick = startTick + ppq;

                    if (event->noteOffObject != nullptr)
                        endTick = static_cast<int>(event->noteOffObject->message.getTimeStamp());

                    channelTrack.addNote({noteNumber, velocity, startTick, endTick - startTick});
                }
                else if (msg.isController())
                {
                    auto& channelTrack = trackForChannel(msg.getChannel());

                    channelTrack.addEvent({.type = MidiEvent::Type::ControlChange,
                                           .tick = static_cast<int>(msg.getTimeStamp()),
                                           .data1 = msg.getControllerNumber(),
                                           .data2 = msg.getControllerValue()});
                }
                else if (msg.isProgramChange())
                {
                    auto& channelTrack = trackForChannel(msg.getChannel());

                    channelTrack.addEvent({.type = MidiEvent::Type::ProgramChange,
                                           .tick = static_cast<int>(msg.getTimeStamp()),
                                           .data1 = msg.getProgramChangeNumber()});
                }
                else if (msg.isPitchWheel())
                {
                    auto& channelTrack = trackForChannel(msg.getChannel());

                    channelTrack.addEvent({.type = MidiEvent::Type::PitchBend,
                                           .tick = static_cast<int>(msg.getTimeStamp()),
                                           .data1 = msg.getPitchWheelValue()});
                }
                else if (msg.isChannelPressure())
                {
                    auto& channelTrack = trackForChannel(msg.getChannel());

                    channelTrack.addEvent({.type = MidiEvent::Type::ChannelPressure,
                                           .tick = static_cast<int>(msg.getTimeStamp()),
                                           .data1 = msg.getChannelPressureValue()});
                }
                else if (msg.isAftertouch())
                {
                    auto& channelTrack = trackForChannel(msg.getChannel());

                    channelTrack.addEvent({.type = MidiEvent::Type::KeyPressure,
                                           .tick = static_cast<int>(msg.getTimeStamp()),
                                           .data1 = msg.getNoteNumber(),
                                           .data2 = msg.getAfterTouchValue()});
                }
                else if (const auto sysEx = readSysEx(msg))
                {
                    ensureTrack().addSysEx(*sysEx);
                }
                else if (const auto metaEvent = readMetaEvent(msg, metaOwner))
                {
                    addMetaEvent(*metaEvent);
                }
            }

            // By convention the first track of a format 1 file is the conductor track, so it becomes a track only when
            // it has channel events or SysEx.
            if (!track && t > 0)
            {
                track = &contents.tracks.emplace_back();
            }

            if (track && !firstChannel && prefixChannel)
                track->setChannel(*prefixChannel);

            if (track && trackName.isNotEmpty())
                track->setName(trackName.toStdString());
        }
    }

    if (contents.tracks.empty())
        contents.tracks.emplace_back();

    for (auto& track : contents.tracks)
        track.setEvents(convertStepsToPoints(track.getEvents()));

    // A song the project file would reject could not be reopened after saving.
    if (!SequenceContentsRules::accepts(contents))
        return std::nullopt;
    return contents;
}
