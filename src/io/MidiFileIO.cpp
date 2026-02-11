#include "MidiFileIO.h"

bool MidiFileIO::save(const MidiSequence& sequence, const juce::File& file)
{
    juce::MidiFile midiFile;
    int ppq = sequence.getTicksPerQuarterNote();
    midiFile.setTicksPerQuarterNote(ppq);

    juce::MidiMessageSequence tempoTrack;
    int microsecondsPerBeat = static_cast<int>(60000000.0 / sequence.getBpm());
    auto tempoEvent = juce::MidiMessage::tempoMetaEvent(microsecondsPerBeat);
    tempoEvent.setTimeStamp(0);
    tempoTrack.addEvent(tempoEvent);

    auto endOfTempoTrack = juce::MidiMessage::endOfTrack();
    endOfTempoTrack.setTimeStamp(0);
    tempoTrack.addEvent(endOfTempoTrack);
    midiFile.addTrack(tempoTrack);

    for (int t = 0; t < sequence.getNumTracks(); ++t)
    {
        const auto& track = sequence.getTrack(t);
        juce::MidiMessageSequence msgSeq;
        int lastTick = 0;

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);

            auto noteOn =
                juce::MidiMessage::noteOn(note.channel, note.noteNumber, static_cast<juce::uint8>(note.velocity));
            noteOn.setTimeStamp(note.startTick);
            msgSeq.addEvent(noteOn);

            auto noteOff = juce::MidiMessage::noteOff(note.channel, note.noteNumber);
            noteOff.setTimeStamp(note.endTick());
            msgSeq.addEvent(noteOff);

            if (note.endTick() > lastTick)
                lastTick = note.endTick();
        }

        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            const auto& event = track.getEvent(i);
            juce::MidiMessage msg;

            switch (event.type)
            {
            case MidiEvent::Type::ControlChange:
                msg = juce::MidiMessage::controllerEvent(event.channel, event.data1, event.data2);
                break;
            case MidiEvent::Type::ProgramChange:
                msg = juce::MidiMessage::programChange(event.channel, event.data1);
                break;
            case MidiEvent::Type::PitchBend:
                msg = juce::MidiMessage::pitchWheel(event.channel, event.data1);
                break;
            case MidiEvent::Type::ChannelPressure:
                msg = juce::MidiMessage::channelPressureChange(event.channel, event.data1);
                break;
            case MidiEvent::Type::KeyPressure:
                msg = juce::MidiMessage::aftertouchChange(event.channel, event.data1, event.data2);
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

    file.deleteFile();
    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;

    return midiFile.writeTo(stream, 1);
}

bool MidiFileIO::load(MidiSequence& sequence, const juce::File& file)
{
    juce::MemoryBlock fileData;
    if (!file.loadFileAsData(fileData))
        return false;

    auto* data = static_cast<const uint8_t*>(fileData.getData());
    size_t size = fileData.getSize();

    // MThdやMTrk以外の非標準チャンクをフィルタリング（YAMAHA XGファイルのXFIH等）
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

    juce::MemoryInputStream stream(filteredData.getData(), filteredData.getSize(), false);

    juce::MidiFile midiFile;
    if (!midiFile.readFrom(stream))
        return false;

    sequence.clear();

    int ppq = midiFile.getTimeFormat();
    if (ppq <= 0)
        ppq = MidiSequence::defaultTicksPerQuarterNote;
    sequence.setTicksPerQuarterNote(ppq);

    for (int t = 0; t < midiFile.getNumTracks(); ++t)
    {
        const auto* msgSeq = midiFile.getTrack(t);
        for (int i = 0; i < msgSeq->getNumEvents(); ++i)
        {
            const auto& msg = msgSeq->getEventPointer(i)->message;
            if (msg.isTempoMetaEvent())
            {
                double bpm = 60000000.0 / msg.getTempoSecondsPerQuarterNote() / 1000000.0;
                sequence.setBpm(bpm);
                break;
            }
        }
    }

    for (int t = 0; t < midiFile.getNumTracks(); ++t)
    {
        const auto* msgSeq = midiFile.getTrack(t);
        juce::MidiMessageSequence sorted(*msgSeq);
        sorted.updateMatchedPairs();

        MidiTrack* track = nullptr;

        for (int i = 0; i < sorted.getNumEvents(); ++i)
        {
            const auto* event = sorted.getEventPointer(i);
            const auto& msg = event->message;

            if (msg.isNoteOn())
            {
                if (!track)
                    track = &sequence.addTrack();

                int noteNumber = msg.getNoteNumber();
                int velocity = msg.getVelocity();
                int startTick = static_cast<int>(msg.getTimeStamp());
                int endTick = startTick + ppq;
                int channel = msg.getChannel();

                if (event->noteOffObject != nullptr)
                    endTick = static_cast<int>(event->noteOffObject->message.getTimeStamp());

                track->addNote({noteNumber, velocity, startTick, endTick - startTick, channel});
            }
            else if (msg.isController())
            {
                if (!track)
                    track = &sequence.addTrack();

                MidiEvent ev;
                ev.type = MidiEvent::Type::ControlChange;
                ev.channel = msg.getChannel();
                ev.tick = static_cast<int>(msg.getTimeStamp());
                ev.data1 = msg.getControllerNumber();
                ev.data2 = msg.getControllerValue();
                track->addEvent(ev);
            }
            else if (msg.isProgramChange())
            {
                if (!track)
                    track = &sequence.addTrack();

                MidiEvent ev;
                ev.type = MidiEvent::Type::ProgramChange;
                ev.channel = msg.getChannel();
                ev.tick = static_cast<int>(msg.getTimeStamp());
                ev.data1 = msg.getProgramChangeNumber();
                track->addEvent(ev);
            }
            else if (msg.isPitchWheel())
            {
                if (!track)
                    track = &sequence.addTrack();

                MidiEvent ev;
                ev.type = MidiEvent::Type::PitchBend;
                ev.channel = msg.getChannel();
                ev.tick = static_cast<int>(msg.getTimeStamp());
                ev.data1 = msg.getPitchWheelValue();
                track->addEvent(ev);
            }
            else if (msg.isChannelPressure())
            {
                if (!track)
                    track = &sequence.addTrack();

                MidiEvent ev;
                ev.type = MidiEvent::Type::ChannelPressure;
                ev.channel = msg.getChannel();
                ev.tick = static_cast<int>(msg.getTimeStamp());
                ev.data1 = msg.getChannelPressureValue();
                track->addEvent(ev);
            }
            else if (msg.isAftertouch())
            {
                if (!track)
                    track = &sequence.addTrack();

                MidiEvent ev;
                ev.type = MidiEvent::Type::KeyPressure;
                ev.channel = msg.getChannel();
                ev.tick = static_cast<int>(msg.getTimeStamp());
                ev.data1 = msg.getNoteNumber();
                ev.data2 = msg.getAfterTouchValue();
                track->addEvent(ev);
            }
        }
    }

    if (sequence.getNumTracks() == 0)
        sequence.addTrack();

    return true;
}
