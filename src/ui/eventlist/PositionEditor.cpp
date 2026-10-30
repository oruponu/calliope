#include "ui/eventlist/PositionEditor.h"
#include "ui/eventlist/EventListPosition.h"
#include "ui/theme/Theme.h"

namespace
{
constexpr int kBarWidth = 26;
constexpr int kBeatWidth = 18;
constexpr int kDotWidth = 4;

constexpr std::array kFields{PositionField::Bar, PositionField::Beat, PositionField::Tick};

struct FieldFormat
{
    int maxLength;
    int paddedLength;
};

FieldFormat formatOf(PositionField field)
{
    switch (field)
    {
    case PositionField::Bar:
        return {4, 3};
    case PositionField::Beat:
        return {2, 2};
    case PositionField::Tick:
        return {5, 4};
    }
    return {};
}
} // namespace

PositionEditor::PositionEditor(const TimelineMap& timelineRef, const BarBeatTick& position)
    : timeline(timelineRef), draft(position)
{
    using namespace calliope::theme;
    for (const auto field : kFields)
    {
        auto& editor = editorOf(field);
        editor.setFont(font::sans(font::sizeSM));
        editor.setIndents(0, 0);
        editor.setJustification(juce::Justification::centred);
        editor.setInputRestrictions(formatOf(field).maxLength, "0123456789");
        editor.setSelectAllWhenFocused(true);
        editor.onReturnKey = [this]() { commit(Exit::ReturnKey); };
        editor.onEscapeKey = [this]()
        {
            ended = true;
            if (onCancel)
                onCancel();
        };
        // Arrives after the focus has moved, so moving between the boxes does not end the edit.
        editor.onFocusLost = [this]()
        {
            if (!hasKeyboardFocus(true))
                commit(Exit::FocusLost);
        };
        addAndMakeVisible(editor);
    }
    showDraft();
}

void PositionEditor::focusField(PositionField field)
{
    editorOf(field).grabKeyboardFocus();
}

PositionField PositionEditor::fieldAt(int x)
{
    if (x < kBarWidth + kDotWidth / 2)
        return PositionField::Bar;
    if (x < kBarWidth + kDotWidth + kBeatWidth + kDotWidth / 2)
        return PositionField::Beat;
    return PositionField::Tick;
}

void PositionEditor::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    // The row's own text would show between the boxes.
    g.fillAll(editorOf(PositionField::Bar).findColour(juce::TextEditor::backgroundColourId));
    g.setColour(text::t2);
    g.setFont(font::sans(font::sizeSM));
    for (const auto field : {PositionField::Bar, PositionField::Beat})
        g.drawText(".", editorOf(field).getRight(), 0, kDotWidth, getHeight(), juce::Justification::centred);
}

void PositionEditor::resized()
{
    auto area = getLocalBounds();
    editorOf(PositionField::Bar).setBounds(area.removeFromLeft(kBarWidth));
    area.removeFromLeft(kDotWidth);
    editorOf(PositionField::Beat).setBounds(area.removeFromLeft(kBeatWidth));
    area.removeFromLeft(kDotWidth);
    editorOf(PositionField::Tick).setBounds(area);
}

juce::TextEditor& PositionEditor::editorOf(PositionField field)
{
    return editors[static_cast<size_t>(field)];
}

// onTextChange arrives later than a key that ends the edit at once, so the boxes are read when the draft is used.
void PositionEditor::readFields()
{
    for (const auto field : kFields)
    {
        const auto text = editorOf(field).getText();
        if (text.isEmpty())
            continue;
        const int value = text.getIntValue();
        switch (field)
        {
        case PositionField::Bar:
            draft.bar = value;
            break;
        case PositionField::Beat:
            draft.beat = value;
            break;
        case PositionField::Tick:
            draft.tick = value;
            break;
        }
    }
}

void PositionEditor::showDraft()
{
    for (const auto field : kFields)
    {
        const int value = field == PositionField::Bar    ? draft.bar
                          : field == PositionField::Beat ? draft.beat
                                                         : draft.tick;
        auto& editor = editorOf(field);
        editor.setText(juce::String(value).paddedLeft('0', formatOf(field).paddedLength), juce::dontSendNotification);
        if (editor.hasKeyboardFocus(false))
            editor.selectAll();
    }
}

void PositionEditor::commit(Exit exit)
{
    if (ended)
        return;
    ended = true;
    readFields();
    if (onCommit)
        onCommit(EventListPosition::toTick(draft, timeline), exit);
}
