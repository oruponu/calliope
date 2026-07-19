#include "TimeSignatureEditor.h"
#include "Theme.h"

TimeSignatureEditor::TimeSignatureEditor(int numerator, int denominator, bool startNumeratorEdit)
    : draftNum(clampNumerator(numerator)), draftDen(snapDenominator(denominator)),
      startNumeratorEdit(startNumeratorEdit)
{
    using namespace calliope::theme;

    struct Field
    {
        WheelLabel* label;
        juce::Justification justification;
    };
    for (auto field :
         {Field{&numLabel, juce::Justification::centredRight}, Field{&denLabel, juce::Justification::centredLeft}})
    {
        auto* label = field.label;
        auto justification = field.justification;
        addAndMakeVisible(label);
        label->setFont(font::mono(font::sizeXL).boldened());
        label->setColour(juce::Label::textColourId, text::t1);
        label->setJustificationType(justification);
        label->setBorderSize(juce::BorderSize<int>(0));
        label->setMinimumHorizontalScale(1.0f);
        label->setEditable(true);
        label->setWantsKeyboardFocus(false);
        label->onEditorShow = [label, justification]()
        {
            if (auto* editor = label->getCurrentTextEditor())
            {
                editor->setInputRestrictions(2, "0123456789");
                editor->setJustification(justification);
                editor->selectAll();
            }
        };
        label->onEditorHide = [this]()
        {
            juce::MessageManager::callAsync(
                [safe = juce::Component::SafePointer<TimeSignatureEditor>(this)]()
                {
                    if (safe != nullptr)
                        safe->grabKeyboardFocus();
                });
        };
    }

    numLabel.onTextChange = [this]()
    { setDraft(numLabel.getText().isEmpty() ? draftNum : numLabel.getText().getIntValue(), draftDen); };
    denLabel.onTextChange = [this]()
    { setDraft(draftNum, denLabel.getText().isEmpty() ? draftDen : denLabel.getText().getIntValue()); };
    numLabel.onWheel = [this](int direction) { setDraft(draftNum + direction, draftDen); };
    denLabel.onWheel = [this](int direction) { setDraft(draftNum, direction > 0 ? draftDen * 2 : draftDen / 2); };

    addAndMakeVisible(slashLabel);
    slashLabel.setText("/", juce::dontSendNotification);
    slashLabel.setFont(font::mono(font::sizeXL));
    slashLabel.setColour(juce::Label::textColourId, text::t2);
    slashLabel.setJustificationType(juce::Justification::centred);
    slashLabel.setBorderSize(juce::BorderSize<int>(0));
    slashLabel.setInterceptsMouseClicks(false, false);

    refreshLabels();
    setWantsKeyboardFocus(true);
    setSize(96, 32);
}

TimeSignatureEditor::~TimeSignatureEditor()
{
    finalize(true);
}

void TimeSignatureEditor::abandon()
{
    finalized = true;
    onDraftChanged = nullptr;
    onCommit = nullptr;
    onCancel = nullptr;
}

void TimeSignatureEditor::resized()
{
    auto area = getLocalBounds();
    numLabel.setBounds(area.removeFromLeft(38));
    slashLabel.setBounds(area.removeFromLeft(20));
    denLabel.setBounds(area);
}

bool TimeSignatureEditor::keyPressed(const juce::KeyPress& key)
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

void TimeSignatureEditor::parentHierarchyChanged()
{
    if (!isShowing() || !initialFocusPending)
        return;

    initialFocusPending = false;
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<TimeSignatureEditor>(this)]()
        {
            if (safe == nullptr)
                return;
            if (safe->startNumeratorEdit)
                safe->numLabel.showEditor();
            else
                safe->grabKeyboardFocus();
        });
}

int TimeSignatureEditor::clampNumerator(int value)
{
    return juce::jlimit(1, 64, value);
}

int TimeSignatureEditor::snapDenominator(int value)
{
    value = juce::jlimit(2, 64, value);
    int lower = 1;
    while (lower * 2 <= value)
        lower *= 2;
    int upper = juce::jmin(64, lower * 2);
    return (value - lower <= upper - value) ? lower : upper;
}

void TimeSignatureEditor::setDraft(int num, int den)
{
    num = clampNumerator(num);
    den = snapDenominator(den);
    bool changed = (num != draftNum || den != draftDen);
    draftNum = num;
    draftDen = den;
    refreshLabels();
    if (changed && onDraftChanged)
        onDraftChanged(draftNum, draftDen);
}

void TimeSignatureEditor::refreshLabels()
{
    numLabel.setText(juce::String(draftNum), juce::dontSendNotification);
    denLabel.setText(juce::String(draftDen), juce::dontSendNotification);
}

void TimeSignatureEditor::finalize(bool commit)
{
    if (finalized)
        return;
    finalized = true;
    if (commit)
    {
        if (onCommit)
            onCommit(draftNum, draftDen);
    }
    else if (onCancel)
    {
        onCancel();
    }
}

void TimeSignatureEditor::dismissBox()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}
