#include "session/LayeredNoteEdits.h"
#include "undo/NoteActions.h"
#include <utility>

namespace
{
bool sameNote(const MidiNote& a, const MidiNote& b)
{
    return a.noteNumber == b.noteNumber && a.velocity == b.velocity && a.startTick == b.startTick &&
           a.duration == b.duration;
}
} // namespace

LayeredNoteEdits::LayeredNoteEdits(MidiSequence& sequenceRef, UndoHistory& historyRef, RecordingLayerEditor* editorRef)
    : sequence(sequenceRef), history(historyRef), editor(editorRef)
{
}

void LayeredNoteEdits::apply(const std::vector<NoteEdit>& edits, const juce::String& transactionName)
{
    std::vector<NoteModification> modifications;
    std::vector<RecordedNoteEdit> recorded;
    for (const auto& edit : edits)
    {
        if (edit.ref.isRecorded())
        {
            recorded.push_back({edit.ref.noteIndex, edit.fields});
            continue;
        }
        const auto& before = sequence.getTrack(edit.ref.trackIndex).getNote(edit.ref.noteIndex);
        const auto after = edit.fields.appliedTo(before);
        if (!sameNote(before, after))
            modifications.push_back({edit.ref.trackIndex, edit.ref.noteIndex, before, after});
    }

    if (!modifications.empty())
    {
        history.beginNewTransaction(transactionName);
        history.perform(new MultiNoteModifyAction(&sequence, std::move(modifications)));
    }
    if (!recorded.empty() && editor != nullptr)
        editor->editRecordedNotes(recorded);
}

void LayeredNoteEdits::remove(const std::set<NoteRef>& refs, const juce::String& transactionName)
{
    std::vector<NoteRef> fromSequence;
    std::vector<int> recorded;
    for (const auto& ref : refs)
    {
        if (ref.isRecorded())
            recorded.push_back(ref.noteIndex);
        else
            fromSequence.push_back(ref);
    }

    if (!fromSequence.empty())
    {
        history.beginNewTransaction(transactionName);
        history.perform(new MultiNoteDeleteAction(&sequence, fromSequence));
    }
    if (!recorded.empty() && editor != nullptr)
        editor->removeRecordedNotes(recorded);
}
