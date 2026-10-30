#include "engine/recording/TakeBuilder.h"
#include "engine/recording/HeldTakeNote.h"
#include "engine/recording/InputQuantize.h"
#include "model/event/ChannelVoiceStatus.h"
#include "model/grid/TimelineGrid.h"
#include <algorithm>
#include <array>
#include <limits>
#include <utility>

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
    releasedPending.clear();
    pedalsDown.clear();
    recordedPedals.clear();
    countInNotes.clear();
    countInValues.clear();
    take.clear();
    ++contentRevision;
    ++noteRevision;
}

void TakeBuilder::handle(const std::string& sourceId, const juce::MidiMessage& message, ClockPosition at,
                         const TimelineGrid* grid, LoopRange loop)
{
    if (at.pass < pass || !isChannelVoiceStatus(message.getRawData()[0]))
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
        HeldNote held{.track = *target,
                      .noteNumber = message.getNoteNumber(),
                      .velocity = message.getVelocity(),
                      .startTick = at.tick,
                      .id = nextNoteId++,
                      .pass = pass,
                      .playedTick = at.tick};
        if (grid != nullptr)
        {
            held.pending = true;
            if (const auto start = quantizeInputStart(at.tick, *grid, loop))
            {
                held.startTick = *start;
                held.endOffset = *start - at.tick;
            }
            else
            {
                held.pass = pass + 1;
            }
        }
        heldNotes[key] = held;
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

void TakeBuilder::collectCountIn(const std::string& sourceId, const juce::MidiMessage& message)
{
    if (!isChannelVoiceStatus(message.getRawData()[0]))
        return;

    const int channel = message.getChannel();
    if (message.isNoteOn())
    {
        countInNotes.insert_or_assign(NoteKey{sourceId, channel, message.getNoteNumber()}, message);
        return;
    }
    if (message.isNoteOff())
    {
        countInNotes.erase(NoteKey{sourceId, channel, message.getNoteNumber()});
        return;
    }

    const MidiEvent event = toEvent(message, 0);
    const bool numbered = event.type == MidiEvent::Type::ControlChange || event.type == MidiEvent::Type::KeyPressure;
    const CountInValueKey key{sourceId, channel, static_cast<int>(event.type), numbered ? event.data1 : 0};
    std::erase_if(countInValues, [&](const CountInValue& value) { return value.key == key; });
    countInValues.push_back({key, message});
}

void TakeBuilder::endCountIn(int startTick, const TimelineGrid* grid, LoopRange loop)
{
    const auto values = std::exchange(countInValues, {});
    const auto notes = std::exchange(countInNotes, {});
    const ClockPosition at{pass, startTick};
    for (const auto& [key, message] : values)
    {
        const std::string& sourceId = std::get<0>(key);
        // The pressure of a released key would outlive its note, which is dropped.
        if (message.isAftertouch() && !notes.contains(NoteKey{sourceId, std::get<1>(key), message.getNoteNumber()}))
            continue;
        handle(sourceId, message, at, grid, loop);
    }
    for (const auto& [key, message] : notes)
        handle(std::get<0>(key), message, at, grid, loop);
}

bool TakeBuilder::reach(ClockPosition sent)
{
    bool changed = false;
    for (auto& [key, held] : heldNotes)
        if (held.pending && hasReached(held.pass, held.startTick, sent))
        {
            held.pending = false;
            changed = true;
        }

    std::vector<PendingNote> stillPending;
    for (auto& released : releasedPending)
    {
        if (hasReached(released.pass, released.note.startTick, sent))
        {
            addNote(released.track, released.note, released.id);
            changed = true;
        }
        else
        {
            stillPending.push_back(std::move(released));
        }
    }
    releasedPending = std::move(stillPending);
    return changed;
}

bool TakeBuilder::reachPassEnds(const std::vector<ClockPosition>& passEnds)
{
    bool changed = false;
    std::vector<PendingNote> stillPending;
    for (auto& released : releasedPending)
    {
        const auto end = std::ranges::find(passEnds, released.pass, &ClockPosition::pass);
        if (released.pass >= pass || end == passEnds.end())
        {
            stillPending.push_back(std::move(released));
        }
        else if (released.note.startTick < end->tick)
        {
            addNote(released.track, released.note, released.id);
            changed = true;
        }
    }
    releasedPending = std::move(stillPending);
    return changed;
}

void TakeBuilder::setTarget(std::optional<TrackId> newTarget, int tick)
{
    if (newTarget == target)
        return;
    closeAllNotes(tick);
    releasedPending.clear();
    releaseAllPedals(tick);
    target = newTarget;
}

void TakeBuilder::closeHeld(const PassBoundary& boundary, const PlaybackProgress& progress)
{
    reachPassEnds(progress.passEnds);
    // Only what playback actually sent of this pass: a wrap announced by input may never be taken.
    if (const auto sentTick = progress.sentTickIn(pass))
        reach({pass, *sentTick});
    // Playback has not taken the wrap yet and still sends the rest of this pass, unless a jump cancels the wrap.
    const bool passStillPlaying =
        boundary.loopWrap && !std::ranges::contains(progress.passEnds, pass, &ClockPosition::pass);

    std::vector<PendingNote> kept;
    for (auto& released : releasedPending)
    {
        if (released.pass > pass && boundary.loopWrap)
        {
            released.note.startTick = boundary.startTick;
            kept.push_back(std::move(released));
        }
        else if (released.pass == pass && passStillPlaying)
        {
            kept.push_back(std::move(released));
        }
    }
    releasedPending = std::move(kept);

    std::vector<NoteKey> waiting;
    std::vector<NoteKey> closing;
    for (auto& [key, held] : heldNotes)
    {
        if (held.pass > pass && boundary.loopWrap)
        {
            held.startTick = boundary.startTick;
            held.endOffset = boundary.closeTick - held.playedTick;
        }
        else if (held.pending && passStillPlaying)
        {
            releasedPending.push_back(
                {std::get<0>(key),
                 held.track,
                 {held.noteNumber, held.velocity, held.startTick, std::max(1, boundary.closeTick - held.startTick)},
                 held.id,
                 held.pass});
            waiting.push_back(key);
        }
        else
        {
            closing.push_back(key);
        }
    }
    for (const auto& key : waiting)
        heldNotes.erase(key);
    for (const auto& key : closing)
        closeNote(heldNotes.find(key), boundary.closeTick);

    // Loop playback sends events before the loop end only and resets no controllers on the wrap, so a release on
    // the loop end itself would never be heard and the pedal would stay down in every later pass.
    releaseAllPedals(boundary.closeTick - 1);
    pass = boundary.pass;
}

void TakeBuilder::releaseSource(const std::string& sourceId, int tick)
{
    std::erase_if(countInNotes, [&](const auto& entry) { return std::get<0>(entry.first) == sourceId; });
    std::erase_if(countInValues, [&](const CountInValue& value) { return std::get<0>(value.key) == sourceId; });

    std::vector<NoteKey> notes;
    for (const auto& [key, held] : heldNotes)
        if (std::get<0>(key) == sourceId)
            notes.push_back(key);
    for (const auto& key : notes)
        closeNote(heldNotes.find(key), tick);
    std::erase_if(releasedPending, [&](const PendingNote& released) { return released.sourceId == sourceId; });

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
    closeAllNotes(tick);
    releasedPending.clear();
    releaseAllPedals(tick);
    target.reset();
    pedalsDown.clear();
    recordedPedals.clear();
    countInNotes.clear();
    countInValues.clear();
    ++contentRevision;
    ++noteRevision;
    return std::exchange(take, {});
}

void TakeBuilder::editNotes(const std::vector<RecordedNoteEdit>& edits, int playheadTick)
{
    for (const auto& edit : edits)
    {
        if (const auto held = findHeld(edit.id); held != heldNotes.end())
        {
            editHeldNote(held, edit.fields, playheadTick);
        }
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
        if (!note.pending)
            held.push_back({note.track, note.noteNumber, note.velocity, note.startTick, note.id});
    return held;
}

void TakeBuilder::finishNote(const NoteKey& key, int tick)
{
    const auto it = heldNotes.find(key);
    if (it == heldNotes.end())
        return;
    const HeldNote held = it->second;
    if (held.pass > pass)
    {
        heldNotes.erase(it);
        releasedPending.push_back({std::get<0>(key),
                                   held.track,
                                   {held.noteNumber, held.velocity, 0, std::max(1, tick - held.playedTick)},
                                   held.id,
                                   held.pass});
        return;
    }

    // SequenceContentsRules keeps every note end inside int.
    const long long end =
        std::min<long long>(static_cast<long long>(tick) + held.endOffset, std::numeric_limits<int>::max());
    const int duration = std::max(1, static_cast<int>(end - held.startTick));
    if (!held.pending)
    {
        endHeldNote(it, held.startTick, duration);
        return;
    }
    heldNotes.erase(it);
    releasedPending.push_back(
        {std::get<0>(key), held.track, {held.noteNumber, held.velocity, held.startTick, duration}, held.id, held.pass});
}

void TakeBuilder::closeNote(HeldNotes::iterator it, int tick)
{
    if (it->second.pending)
    {
        heldNotes.erase(it);
        return;
    }
    endHeldNote(it, it->second.startTick, std::max(1, tick - it->second.startTick));
}

void TakeBuilder::closeAllNotes(int tick)
{
    std::vector<NoteKey> keys;
    for (const auto& [key, held] : heldNotes)
        keys.push_back(key);
    for (const auto& key : keys)
        closeNote(heldNotes.find(key), tick);
}

void TakeBuilder::endHeldNote(HeldNotes::iterator it, int startTick, int duration)
{
    const HeldNote held = it->second;
    heldNotes.erase(it);
    addNote(held.track, {held.noteNumber, held.velocity, startTick, duration}, held.id);
}

void TakeBuilder::addNote(TrackId track, const MidiNote& note, int id)
{
    auto& recorded = take[track];
    recorded.notes.push_back(note);
    recorded.noteIds.push_back(id);
    ++contentRevision;
    ++noteRevision;
}

bool TakeBuilder::hasReached(int notePass, int startTick, ClockPosition sent) const
{
    // A note carried over the loop end has no start until the loop wraps, and a note left behind by an announced wrap
    // waits for reachPassEnds.
    if (notePass != pass)
        return false;
    return sent.pass > notePass || (sent.pass == notePass && startTick < sent.tick);
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
    {
        endHeldNote(it, *edit.startTick, std::max(1, edit.heldLength.value_or(playheadTick - held.startTick)));
    }
    else if (edit.startTick)
    {
        held.startTick = *edit.startTick;
        held.endOffset = 0;
    }
}

TakeBuilder::HeldNotes::iterator TakeBuilder::findHeld(int id)
{
    return std::ranges::find_if(heldNotes,
                                [id](const auto& entry) { return entry.second.id == id && !entry.second.pending; });
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
