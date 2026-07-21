#include "ui/pianoroll/strips/KeySignatureEditor.h"
#include "model/MidiSequence.h"
#include "ui/theme/Theme.h"

KeySignatureEditor::KeySignatureEditor(int sharpsOrFlats, bool isMinor, bool startTextEdit)
    : draftSharpsOrFlats(MidiSequence::normalizeSharpsOrFlats(sharpsOrFlats)), draftIsMinor(isMinor),
      startTextEdit(startTextEdit)
{
    using namespace calliope::theme;

    addAndMakeVisible(keyLabel);
    keyLabel.setFont(font::mono(font::sizeXL).boldened());
    keyLabel.setColour(juce::Label::textColourId, text::t1);
    keyLabel.setJustificationType(juce::Justification::centred);
    keyLabel.setBorderSize(juce::BorderSize<int>(0));
    keyLabel.setMinimumHorizontalScale(1.0f);
    keyLabel.setEditable(true);
    keyLabel.setWantsKeyboardFocus(false);
    keyLabel.onEditorShow = [this]()
    {
        if (auto* editor = keyLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(3, "ABCDEFGabcdefg#bMm");
            editor->setJustification(juce::Justification::centred);
            editor->selectAll();
        }
    };
    keyLabel.onEditorHide = [this]()
    {
        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<KeySignatureEditor>(this)]()
            {
                if (safe != nullptr)
                    safe->grabKeyboardFocus();
            });
    };
    keyLabel.onTextChange = [this]()
    {
        int sf = 0;
        bool minor = false;
        if (MidiSequence::keySignatureFromString(keyLabel.getText().toStdString(), sf, minor))
            setDraft(sf, minor);
        else
            refreshLabel();
    };
    keyLabel.onWheel = [this](int direction) { nudgeDraft(direction); };

    refreshLabel();
    setWantsKeyboardFocus(true);
    setSize(72, 32);
}

KeySignatureEditor::~KeySignatureEditor()
{
    finalize(true);
}

void KeySignatureEditor::abandon()
{
    finalized = true;
    onDraftChanged = nullptr;
    onCommit = nullptr;
    onCancel = nullptr;
}

void KeySignatureEditor::resized()
{
    keyLabel.setBounds(getLocalBounds());
}

bool KeySignatureEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::returnKey)
    {
        finalize(true);
        dismissBox();
        return true;
    }
    if (key == juce::KeyPress::escapeKey)
    {
        finalize(false);
        dismissBox();
        return true;
    }
    return false;
}

void KeySignatureEditor::parentHierarchyChanged()
{
    if (!isShowing() || !initialFocusPending)
        return;

    initialFocusPending = false;
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<KeySignatureEditor>(this)]()
        {
            if (safe == nullptr)
                return;
            if (safe->startTextEdit)
                safe->keyLabel.showEditor();
            else
                safe->grabKeyboardFocus();
        });
}

void KeySignatureEditor::setDraft(int sharpsOrFlats, bool isMinor)
{
    sharpsOrFlats = MidiSequence::normalizeSharpsOrFlats(sharpsOrFlats);
    bool changed = (sharpsOrFlats != draftSharpsOrFlats || isMinor != draftIsMinor);
    draftSharpsOrFlats = sharpsOrFlats;
    draftIsMinor = isMinor;
    refreshLabel();
    if (changed && onDraftChanged)
        onDraftChanged(draftSharpsOrFlats, draftIsMinor);
}

void KeySignatureEditor::nudgeDraft(int direction)
{
    int sf = juce::jlimit(-6, 6, draftSharpsOrFlats);
    int index = juce::jlimit(0, 25, (sf + 6) + (draftIsMinor ? 13 : 0) + direction);
    setDraft((index >= 13 ? index - 13 : index) - 6, index >= 13);
}

void KeySignatureEditor::refreshLabel()
{
    keyLabel.setText(juce::String(MidiSequence::keySignatureToString(draftSharpsOrFlats, draftIsMinor)),
                     juce::dontSendNotification);
}

void KeySignatureEditor::finalize(bool commit)
{
    if (finalized)
        return;
    finalized = true;
    if (commit)
    {
        if (onCommit)
            onCommit(draftSharpsOrFlats, draftIsMinor);
    }
    else if (onCancel)
    {
        onCancel();
    }
}

void KeySignatureEditor::dismissBox()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}
