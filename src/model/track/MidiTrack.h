#pragma once

#include "model/event/MetaEvent.h"
#include "model/event/MidiEvent.h"
#include "model/event/MidiNote.h"
#include "model/event/SysExEvent.h"
#include "model/track/PluginAssignment.h"
#include "model/track/TrackId.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class MidiTrack
{
public:
    MidiTrack() = default;
    explicit MidiTrack(TrackId provisionalId);

    enum class OutputDestination
    {
        MidiDevice,
        Plugin,
        None
    };

    TrackId getId() const;

    void addNote(const MidiNote& note);
    void insertNote(int index, const MidiNote& note);
    void removeNote(int index);
    void clear();
    void sortByStartTime();

    const std::vector<MidiNote>& getNotes() const;
    MidiNote& getNote(int index);
    const MidiNote& getNote(int index) const;
    int getNumNotes() const;

    void addEvent(const MidiEvent& event);
    void insertEvent(int index, const MidiEvent& event);
    void removeEvent(int index);
    void setEvents(std::vector<MidiEvent> newEvents);
    const std::vector<MidiEvent>& getEvents() const;
    const MidiEvent& getEvent(int index) const;
    int getNumEvents() const;

    void addSysEx(const SysExEvent& sysEx);
    const std::vector<SysExEvent>& getSysExEvents() const;

    void addMetaEvent(const MetaEvent& event);
    const std::vector<MetaEvent>& getMetaEvents() const;

    bool isMuted() const;
    void setMuted(bool muted);

    bool isSolo() const;
    void setSolo(bool solo);

    const std::string& getName() const;
    void setName(const std::string& name);

    int getChannel() const;
    void setChannel(int channel);

    OutputDestination getOutputDestination() const;
    void setOutputDestination(OutputDestination dest);

    const std::optional<TrackId>& getRouteTarget() const;
    void setRouteTarget(std::optional<TrackId> target);

    const std::shared_ptr<const PluginAssignment>& getPluginAssignment() const;
    void setPluginAssignment(std::shared_ptr<const PluginAssignment> assignment);

private:
    friend class MidiSequence;

    TrackId id{};
    std::vector<MidiNote> notes;
    std::vector<MidiEvent> events;
    std::vector<SysExEvent> sysExEvents;
    std::vector<MetaEvent> metaEvents;
    std::string name;
    bool muted = false;
    bool solo = false;
    int channel = 1;
    OutputDestination outputDestination = OutputDestination::MidiDevice;
    std::optional<TrackId> routeTarget;
    std::shared_ptr<const PluginAssignment> pluginAssignment;
};
