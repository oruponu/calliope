#pragma once

#include "engine/recording/RecordedNoteEdit.h"
#include <vector>

// Changes the recording segment in progress. The changes reach the undo history only inside the "Record" step at
// commit.
class RecordingLayerEditor
{
public:
    virtual ~RecordingLayerEditor() = default;
    virtual void editRecordedNotes(const std::vector<RecordedNoteEdit>& edits) = 0;
    virtual void removeRecordedNotes(const std::vector<int>& ids) = 0;
};
