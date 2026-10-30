#pragma once

#include "ui/eventlist/EditorExit.h"
#include "ui/eventlist/FieldTextEditor.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// The wheel steps a semitone and reports the note number at once; typed text only reaches onCommit when the edit ends,
// with the note number it started from when the text is not a note.
class NoteEditor : public juce::Component
{
public:
    explicit NoteEditor(int noteNumber);

    std::function<void(int noteNumber)> onWheelStep;
    // Called once when the edit ends, unless onCancel is. Either may destroy the editor.
    std::function<void(int noteNumber, EditorExit exit)> onCommit;
    std::function<void()> onCancel;

    void focusField();

    void resized() override;

private:
    void readField();
    void showDraft();
    void commit(EditorExit exit);

    const int original;
    int draft;
    FieldTextEditor editor;
    bool ended = false;
};
