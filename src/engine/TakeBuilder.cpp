#include "engine/TakeBuilder.h"
#include "engine/ChannelVoiceMessage.h"
#include <algorithm>
#include <array>

namespace
{
constexpr std::array holdPedalControllers{64, 66, 69};

bool isHoldPedal(int controller)
{
    return std::ranges::contains(holdPedalControllers, controller);
}

MidiEvent toEvent(const juce::MidiMessage& message, int tick)
{
    if (message.isController())
        return {.type = MidiEvent::Type::ControlChange,
                .tick = tick,
                .data1 = message.getControllerNumber(),
                .data2 = message.getControllerValue()};
    if (message.isProgramChange())
        return {.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = message.getProgramChangeNumber()};
    if (message.isPitchWheel())
        return {.type = MidiEvent::Type::PitchBend, .tick = tick, .data1 = message.getPitchWheelValue()};
    if (message.isChannelPressure())
        return {.type = MidiEvent::Type::ChannelPressure, .tick = tick, .data1 = message.getChannelPressureValue()};
    return {.type = MidiEvent::Type::KeyPressure,
            .tick = tick,
            .data1 = message.getNoteNumber(),
            .data2 = message.getAfterTouchValue()};
}
} // namespace

void TakeBuilder::begin(std::optional<TrackId> newTarget, ClockPosition start)
{
    target = newTarget;
    pass = start.pass;
    heldNotes.clear();
    pedalsDown.clear();
    recordedPedals.clear();
    take.clear();
    ++contentRevision;
    ++noteRevision;
}

void TakeBuilder::handle(const std::string& sourceId, const juce::MidiMessage& message, ClockPosition at)
{
    if (at.pass < pass || !isChannelVoiceMessage(message))
        return;

    const int channel = message.getChannel();
    const bool isPedal = message.isController() && isHoldPedal(message.getControllerNumber());
    if (isPedal)
    {
        const PedalKey key{sourceId, channel, message.getControllerNumber()};
        if (message.getControllerValue() >= 64)
            pedalsDown.insert(key);
        else
            pedalsDown.erase(key);
    }

    if (!target)
        return;

    if (message.isNoteOn())
    {
        const NoteKey key{sourceId, channel, message.getNoteNumber()};
        finishNote(key, at.tick);
        heldNotes[key] = {*target, message.getNoteNumber(), message.getVelocity(), at.tick, nextNoteId++};
        return;
    }
    if (message.isNoteOff())
    {
        finishNote({sourceId, channel, message.getNoteNumber()}, at.tick);
        return;
    }

    if (isPedal)
        recordedPedals[{*target, message.getControllerNumber()}] = {message.getControllerValue(), at.tick};
    addEvent(*target, toEvent(message, at.tick));
}

void TakeBuilder::setTarget(std::optional<TrackId> newTarget, int tick)
{
    if (newTarget == target)
        return;
    finishAllNotes(tick);
    releaseAllPedals(tick);
    target = newTarget;
}

void TakeBuilder::closeHeld(int newPass, int closeTick)
{
    finishAllNotes(closeTick);
    // Loop playback sends events before the loop end only and resets no controllers on the wrap, so a release on
    // the loop end itself would never be heard and the pedal would stay down in every later pass.
    releaseAllPedals(closeTick - 1);
    pass = newPass;
}

void TakeBuilder::releaseSource(const std::string& sourceId, int tick)
{
    std::vector<NoteKey> notes;
    for (const auto& [key, held] : heldNotes)
        if (std::get<0>(key) == sourceId)
            notes.push_back(key);
    for (const auto& key : notes)
        finishNote(key, tick);

    std::set<int> controllers;
    for (auto it = pedalsDown.begin(); it != pedalsDown.end();)
    {
        if (std::get<0>(*it) != sourceId)
        {
            ++it;
            continue;
        }
        controllers.insert(std::get<2>(*it));
        it = pedalsDown.erase(it);
    }
    if (target)
        for (const int controller : controllers)
            releasePedal(*target, controller, tick);
}

Take TakeBuilder::end(int tick)
{
    finishAllNotes(tick);
    releaseAllPedals(tick);
    target.reset();
    pedalsDown.clear();
    recordedPedals.clear();
    ++contentRevision;
    ++noteRevision;
    return std::exchange(take, {});
}

void TakeBuilder::editNotes(const std::vector<RecordedNoteEdit>& edits, int playheadTick)
{
    for (const auto& edit : edits)
    {
        if (const auto held = findHeld(edit.id); held != heldNotes.end())
            editHeldNote(held, edit.fields, playheadTick);
        else if (auto* note = findFinished(edit.id))
        {
            *note = edit.fields.appliedTo(*note);
            ++contentRevision;
            ++noteRevision;
        }
    }
}

void TakeBuilder::removeNotes(const std::vector<int>& ids)
{
    for (const int id : ids)
    {
        if (const auto held = findHeld(id); held != heldNotes.end())
        {
            heldNotes.erase(held);
            continue;
        }
        for (auto& [track, recorded] : take)
        {
            const auto at = std::ranges::find(recorded.noteIds, id);
            if (at == recorded.noteIds.end())
                continue;
            recorded.notes.erase(recorded.notes.begin() + (at - recorded.noteIds.begin()));
            recorded.noteIds.erase(at);
            ++contentRevision;
            ++noteRevision;
            break;
        }
    }
}

std::vector<HeldTakeNote> TakeBuilder::getHeldNotes() const
{
    std::vector<HeldTakeNote> held;
    for (const auto& [key, note] : heldNotes)
        held.push_back({note.track, note.noteNumber, note.velocity, note.startTick, note.id});
    return held;
}

void TakeBuilder::finishNote(const NoteKey& key, int tick)
{
    const auto it = heldNotes.find(key);
    if (it == heldNotes.end())
        return;
    endHeldNote(it, it->second.startTick, std::max(1, tick - it->second.startTick));
}

void TakeBuilder::endHeldNote(HeldNotes::iterator it, int startTick, int duration)
{
    const HeldNote held = it->second;
    heldNotes.erase(it);
    auto& track = take[held.track];
    track.notes.push_back({held.noteNumber, held.velocity, startTick, duration});
    track.noteIds.push_back(held.id);
    ++contentRevision;
    ++noteRevision;
}

void TakeBuilder::finishAllNotes(int tick)
{
    std::vector<NoteKey> keys;
    for (const auto& [key, held] : heldNotes)
        keys.push_back(key);
    for (const auto& key : keys)
        finishNote(key, tick);
}

void TakeBuilder::editHeldNote(HeldNotes::iterator it, const NoteFieldEdit& edit, int playheadTick)
{
    auto& held = it->second;
    if (edit.noteNumber)
        held.noteNumber = *edit.noteNumber;
    if (edit.velocity)
        held.velocity = *edit.velocity;
    if (edit.endTick)
    {
        const int start = edit.startTick.value_or(held.startTick);
        endHeldNote(it, start, std::max(1, *edit.endTick - start));
    }
    else if (edit.startTick && *edit.startTick >= playheadTick)
        endHeldNote(it, *edit.startTick, std::max(1, edit.heldLength.value_or(playheadTick - held.startTick)));
    else if (edit.startTick)
        held.startTick = *edit.startTick;
}

TakeBuilder::HeldNotes::iterator TakeBuilder::findHeld(int id)
{
    return std::ranges::find(heldNotes, id, [](const auto& entry) { return entry.second.id; });
}

MidiNote* TakeBuilder::findFinished(int id)
{
    for (auto& [track, recorded] : take)
        if (const auto at = std::ranges::find(recorded.noteIds, id); at != recorded.noteIds.end())
            return &recorded.notes[static_cast<size_t>(at - recorded.noteIds.begin())];
    return nullptr;
}

void TakeBuilder::releasePedal(TrackId track, int controller, int tick)
{
    auto it = recordedPedals.find({track, controller});
    if (it == recordedPedals.end() || it->second.value < 64)
        return;
    // Never before the press it releases.
    const int releaseTick = std::max(tick, it->second.tick);
    it->second = {0, releaseTick};
    addEvent(track, {.type = MidiEvent::Type::ControlChange, .tick = releaseTick, .data1 = controller, .data2 = 0});
}

void TakeBuilder::releaseAllPedals(int tick)
{
    std::vector<std::pair<TrackId, int>> held;
    for (const auto& [key, pedal] : recordedPedals)
        if (pedal.value >= 64)
            held.push_back(key);
    for (const auto& [track, controller] : held)
        releasePedal(track, controller, tick);
}

void TakeBuilder::addEvent(TrackId track, const MidiEvent& event)
{
    take[track].events.push_back(event);
    ++contentRevision;
}
