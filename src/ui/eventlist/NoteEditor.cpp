#include "ui/eventlist/NoteEditor.h"
#include "model/notation/NoteName.h"
#include "ui/theme/Theme.h"
#include <algorithm>

NoteEditor::NoteEditor(int noteNumber) : original(noteNumber), draft(noteNumber)
{
    using namespace calliope::theme;
    editor.setFont(font::sans(font::sizeSM));
    editor.setIndents(0, 0);
    editor.setJustification(juce::Justification::centredLeft);
    editor.setInputRestrictions(4, "0123456789ABCDEFGabcdefg#-");
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
        draft = std::clamp(draft + direction, 0, 127);
        showDraft();
        if (onWheelStep)
            onWheelStep(draft);
    };
    editor.onTabKey = [this](bool backwards) { commit(backwards ? EditorExit::ShiftTab : EditorExit::Tab); };
    addAndMakeVisible(editor);
    showDraft();
}

void NoteEditor::focusField()
{
    editor.grabKeyboardFocus();
}

void NoteEditor::resized()
{
    editor.setBounds(getLocalBounds());
}

void NoteEditor::readField()
{
    if (const auto noteNumber = NoteName::parse(editor.getText().toStdString()))
        draft = *noteNumber;
}

void NoteEditor::showDraft()
{
    editor.setText(juce::String(NoteName::format(draft)), juce::dontSendNotification);
    if (editor.hasKeyboardFocus(false))
        editor.selectAll();
}

void NoteEditor::commit(EditorExit exit)
{
    if (ended)
        return;
    ended = true;
    const auto typed = NoteName::parse(editor.getText().toStdString());
    if (onCommit)
        onCommit(typed.value_or(original), exit);
}
