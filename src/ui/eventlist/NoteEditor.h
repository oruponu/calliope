#pragma once

#include "ui/eventlist/EditorExit.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// Typed text reaches onCommit when the edit ends, as the note number it started from when the text is not a note.
class NoteEditor : public juce::Component
{
public:
    explicit NoteEditor(int noteNumber);

    // Called once when the edit ends, unless onCancel is. Either may destroy the editor.
    std::function<void(int noteNumber, EditorExit exit)> onCommit;
    std::function<void()> onCancel;

    void focusField();

    void resized() override;

private:
    void showDraft();
    void commit(EditorExit exit);

    const int original;
    int draft;
    juce::TextEditor editor;
    bool ended = false;
};
