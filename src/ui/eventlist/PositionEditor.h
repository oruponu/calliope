#pragma once

#include "model/timeline/BarBeatTick.h"
#include "model/timeline/TimelineMap.h"
#include "ui/eventlist/PositionField.h"
#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class PositionEditor : public juce::Component
{
public:
    enum class Exit
    {
        ReturnKey,
        FocusLost
    };

    PositionEditor(const TimelineMap& timeline, const BarBeatTick& position);

    // Called once when the edit ends, unless onCancel is. Either may destroy the editor.
    std::function<void(int tick, Exit exit)> onCommit;
    std::function<void()> onCancel;

    void focusField(PositionField field);
    // x is relative to the editor's left edge.
    static PositionField fieldAt(int x);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextEditor& editorOf(PositionField field);
    void readFields();
    void showDraft();
    void commit(Exit exit);

    const TimelineMap& timeline;
    BarBeatTick draft;
    std::array<juce::TextEditor, 3> editors;
    bool ended = false;
};
