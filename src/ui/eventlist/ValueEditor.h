#pragma once

#include "ui/eventlist/EditorExit.h"
#include "ui/eventlist/FieldTextEditor.h"
#include "ui/eventlist/ValueFormat.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// The wheel steps the value by one and reports it at once, when onWheelStep is set; typed text only reaches onCommit
// when the edit ends, with the value it started from when the text is not a value.
class ValueEditor : public juce::Component
{
public:
    ValueEditor(int value, ValueFormat format);

    std::function<void(int value)> onWheelStep;
    // Called once when the edit ends, unless onCancel is. Either may destroy the editor.
    std::function<void(int value, EditorExit exit)> onCommit;
    std::function<void()> onCancel;

    void focusField();

    void resized() override;

private:
    void readField();
    void showDraft();
    void commit(EditorExit exit);

    const ValueFormat format;
    const int original;
    int draft;
    FieldTextEditor editor;
    bool ended = false;
};
