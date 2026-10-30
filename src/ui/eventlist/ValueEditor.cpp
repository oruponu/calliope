#include "ui/eventlist/ValueEditor.h"
#include "ui/theme/Theme.h"
#include <algorithm>
#include <utility>

ValueEditor::ValueEditor(int value, ValueFormat formatToUse)
    : format(std::move(formatToUse)), original(value), draft(value)
{
    using namespace calliope::theme;
    editor.setFont(font::sans(font::sizeSM));
    editor.setIndents(0, 0);
    editor.setJustification(format.justification);
    editor.setInputRestrictions(format.maxLength, format.allowedCharacters);
    editor.setSelectAllWhenFocused(true);
    editor.onReturnKey = [this]() { commit(EditorExit::ReturnKey); };
    editor.onEscapeKey = [this]()
    {
        ended = true;
        if (onCancel)
            onCancel();
    };
    editor.onFocusLost = [this]()
    {
        if (!hasKeyboardFocus(true))
            commit(EditorExit::FocusLost);
    };
    editor.onWheel = [this](int direction)
    {
        readField();
        draft = std::clamp(draft + direction, format.minimum, format.maximum);
        showDraft();
        if (onWheelStep)
            onWheelStep(draft);
    };
    editor.onTabKey = [this](bool backwards) { commit(backwards ? EditorExit::ShiftTab : EditorExit::Tab); };
    addAndMakeVisible(editor);
    showDraft();
}

void ValueEditor::focusField()
{
    editor.grabKeyboardFocus();
}

void ValueEditor::resized()
{
    editor.setBounds(getLocalBounds());
}

void ValueEditor::readField()
{
    if (const auto value = format.parse(editor.getText().toStdString()))
        draft = *value;
}

void ValueEditor::showDraft()
{
    editor.setText(juce::String(format.format(draft)), juce::dontSendNotification);
    if (editor.hasKeyboardFocus(false))
        editor.selectAll();
}

void ValueEditor::commit(EditorExit exit)
{
    if (ended)
        return;
    ended = true;
    const auto typed = format.parse(editor.getText().toStdString());
    if (onCommit)
        onCommit(typed.value_or(original), exit);
}
