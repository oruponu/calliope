#pragma once

#include "model/timeline/BarBeatTick.h"
#include "model/timeline/TimelineMap.h"
#include "ui/eventlist/PositionField.h"
#include "ui/eventlist/PositionFieldEditor.h"
#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// The wheel steps the box under it and reports the position at once; typed numbers only reach onCommit when the edit
// ends.
class PositionEditor : public juce::Component
{
public:
    enum class Exit
    {
        ReturnKey,
        FocusLost
    };

    PositionEditor(const TimelineMap& timeline, const BarBeatTick& position);

    std::function<void(int tick)> onWheelStep;
    // Called once when the edit ends, unless onCancel is. Either may destroy the editor.
    std::function<void(int tick, Exit exit)> onCommit;
    std::function<void()> onCancel;

    void focusField(PositionField field);
    // x is relative to the editor's left edge.
    static PositionField fieldAt(int x);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    PositionFieldEditor& editorOf(PositionField field);
    void readFields();
    void showDraft();
    void commit(Exit exit);

    const TimelineMap& timeline;
    BarBeatTick draft;
    std::array<PositionFieldEditor, 3> editors;
    bool ended = false;
};
