#include "ui/pianoroll/strips/ChordEditor.h"
#include "ui/theme/Theme.h"

ChordEditor::ChordEditor(int chordRoot, int chordType, int bassRoot, ChordSpelling spelling, bool startRootEdit)
    : draftRoot(MidiSequence::normalizeChordRoot(chordRoot)), draftType(MidiSequence::normalizeChordType(chordType)),
      draftBassRoot(MidiSequence::normalizeChordBassRoot(bassRoot)), spelling(spelling), startRootEdit(startRootEdit)
{
    using namespace calliope::theme;

    struct Field
    {
        WheelLabel* label;
        juce::Justification justification;
        int maxLength;
        const char* allowedCharacters;
    };
    for (auto field : {Field{&rootLabel, juce::Justification::centredRight, 4, "ABCDEFGabcdefg#b"},
                       Field{&typeLabel, juce::Justification::centredLeft, 8, ""},
                       Field{&bassLabel, juce::Justification::centredLeft, 4, "ABCDEFGabcdefg#b-"}})
    {
        auto* label = field.label;
        auto justification = field.justification;
        auto maxLength = field.maxLength;
        juce::String allowed(field.allowedCharacters);
        addAndMakeVisible(label);
        label->setFont(font::mono(font::sizeXL).boldened());
        label->setColour(juce::Label::textColourId, text::t1);
        label->setJustificationType(justification);
        label->setBorderSize(juce::BorderSize<int>(0));
        label->setMinimumHorizontalScale(1.0f);
        label->setEditable(true);
        label->setWantsKeyboardFocus(false);
        label->onEditorShow = [label, justification, maxLength, allowed]()
        {
            if (auto* editor = label->getCurrentTextEditor())
            {
                editor->setInputRestrictions(maxLength, allowed);
                editor->setJustification(justification);
                editor->selectAll();
            }
        };
        label->onEditorHide = [this]()
        {
            juce::MessageManager::callAsync(
                [safe = juce::Component::SafePointer<ChordEditor>(this)]()
                {
                    if (safe != nullptr)
                        safe->grabKeyboardFocus();
                });
        };
    }

    rootLabel.onTextChange = [this]()
    {
        int root = 0;
        if (MidiSequence::chordRootFromString(rootLabel.getText().toStdString(), root))
            setDraft(root, draftType, draftBassRoot);
        else
            refreshLabels();
    };
    typeLabel.onTextChange = [this]()
    {
        int type = 0;
        if (MidiSequence::chordTypeFromString(typeLabel.getText().toStdString(), type))
            setDraft(draftRoot, type, draftBassRoot);
        else
            refreshLabels();
    };
    bassLabel.onTextChange = [this]()
    {
        auto text = bassLabel.getText().trim();
        if (text.isEmpty() || text == "-")
        {
            setDraft(draftRoot, draftType, MidiSequence::chordNone);
            return;
        }
        int root = 0;
        if (MidiSequence::chordRootFromString(text.toStdString(), root))
            setDraft(draftRoot, draftType, root);
        else
            refreshLabels();
    };

    rootLabel.onWheel = [this](int direction) { nudgeRoot(direction); };
    typeLabel.onWheel = [this](int direction) { nudgeType(direction); };
    bassLabel.onWheel = [this](int direction) { nudgeBass(direction); };

    addAndMakeVisible(slashLabel);
    slashLabel.setText("/", juce::dontSendNotification);
    slashLabel.setFont(font::mono(font::sizeXL));
    slashLabel.setColour(juce::Label::textColourId, text::t2);
    slashLabel.setJustificationType(juce::Justification::centred);
    slashLabel.setBorderSize(juce::BorderSize<int>(0));
    slashLabel.setInterceptsMouseClicks(false, false);

    refreshLabels();
    setWantsKeyboardFocus(true);
    setSize(208, 32);
}

ChordEditor::~ChordEditor()
{
    finalize(true);
}

void ChordEditor::abandon()
{
    finalized = true;
    onDraftChanged = nullptr;
    onCommit = nullptr;
    onCancel = nullptr;
}

void ChordEditor::resized()
{
    auto area = getLocalBounds();
    rootLabel.setBounds(area.removeFromLeft(64));
    typeLabel.setBounds(area.removeFromLeft(64));
    slashLabel.setBounds(area.removeFromLeft(16));
    bassLabel.setBounds(area);
}

bool ChordEditor::keyPressed(const juce::KeyPress& key)
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

void ChordEditor::parentHierarchyChanged()
{
    if (!isShowing() || !initialFocusPending)
        return;

    initialFocusPending = false;
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<ChordEditor>(this)]()
        {
            if (safe == nullptr)
                return;
            if (safe->startRootEdit)
                safe->rootLabel.showEditor();
            else
                safe->grabKeyboardFocus();
        });
}

void ChordEditor::setDraft(int chordRoot, int chordType, int bassRoot)
{
    chordRoot = MidiSequence::normalizeChordRoot(chordRoot);
    chordType = MidiSequence::normalizeChordType(chordType);
    bassRoot = MidiSequence::normalizeChordBassRoot(bassRoot);
    bool changed = (chordRoot != draftRoot || chordType != draftType || bassRoot != draftBassRoot);
    draftRoot = chordRoot;
    draftType = chordType;
    draftBassRoot = bassRoot;
    refreshLabels();
    if (changed && onDraftChanged)
        onDraftChanged(draftRoot, draftType, draftBassRoot);
}

void ChordEditor::nudgeRoot(int direction)
{
    int semitone = MidiSequence::chordRootToSemitone(draftRoot);
    if (semitone < 0)
        semitone = 0;
    setDraft(MidiSequence::semitoneToChordRoot(semitone + direction, spelling), draftType, draftBassRoot);
}

void ChordEditor::nudgeType(int direction)
{
    setDraft(draftRoot, juce::jlimit(0, MidiSequence::chordTypeCount - 1, draftType + direction), draftBassRoot);
}

void ChordEditor::nudgeBass(int direction)
{
    int step = 0;
    if (draftBassRoot != MidiSequence::chordNone)
    {
        int semitone = MidiSequence::chordRootToSemitone(draftBassRoot);
        step = semitone < 0 ? 0 : semitone + 1;
    }
    step = ((step + direction) % 13 + 13) % 13;
    setDraft(draftRoot, draftType,
             step == 0 ? MidiSequence::chordNone : MidiSequence::semitoneToChordRoot(step - 1, spelling));
}

void ChordEditor::refreshLabels()
{
    rootLabel.setText(juce::String(MidiSequence::chordRootToString(draftRoot)), juce::dontSendNotification);
    typeLabel.setText(juce::String(MidiSequence::chordTypeToString(draftType)), juce::dontSendNotification);
    bassLabel.setText(draftBassRoot == MidiSequence::chordNone
                          ? juce::String("-")
                          : juce::String(MidiSequence::chordRootToString(draftBassRoot)),
                      juce::dontSendNotification);
}

void ChordEditor::finalize(bool commit)
{
    if (finalized)
        return;
    finalized = true;
    if (commit)
    {
        if (onCommit)
            onCommit(draftRoot, draftType, draftBassRoot);
    }
    else if (onCancel)
    {
        onCancel();
    }
}

void ChordEditor::dismissBox()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}
