#include "ui/pianoroll/ControllerValueEditor.h"
#include "ui/theme/Theme.h"

namespace
{
bool isInteger(const juce::String& text)
{
    const auto digits = text.startsWithChar('-') ? text.substring(1) : text;
    return digits.isNotEmpty() && digits.containsOnly("0123456789");
}
} // namespace

ControllerValueEditor::ControllerValueEditor(int displayedValue, int minDisplayedIn, int maxDisplayedIn)
    : minDisplayed(minDisplayedIn), maxDisplayed(maxDisplayedIn)
{
    using namespace calliope::theme;

    addAndMakeVisible(valueEditor);
    valueEditor.setFont(font::mono(font::sizeXL).boldened());
    valueEditor.setJustification(juce::Justification::centred);
    valueEditor.setInputRestrictions(6, "-0123456789");
    valueEditor.setText(juce::String(displayedValue), juce::dontSendNotification);
    valueEditor.onReturnKey = [this]()
    {
        finalize(true);
        dismissBox();
    };
    valueEditor.onEscapeKey = [this]()
    {
        finalize(false);
        dismissBox();
    };

    setSize(96, 32);
}

ControllerValueEditor::~ControllerValueEditor()
{
    finalize(false);
}

void ControllerValueEditor::abandon()
{
    finalized = true;
    onCommit = nullptr;
    onCancel = nullptr;
}

void ControllerValueEditor::resized()
{
    valueEditor.setBounds(getLocalBounds());
}

void ControllerValueEditor::parentHierarchyChanged()
{
    if (!isShowing() || !initialFocusPending)
        return;

    initialFocusPending = false;
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<ControllerValueEditor>(this)]()
        {
            if (safe == nullptr)
                return;
            safe->valueEditor.grabKeyboardFocus();
            safe->valueEditor.selectAll();
        });
}

void ControllerValueEditor::finalize(bool commit)
{
    if (finalized)
        return;
    finalized = true;
    const auto text = valueEditor.getText().trim();
    if (commit && isInteger(text))
    {
        if (onCommit)
            onCommit(juce::jlimit(minDisplayed, maxDisplayed, text.getIntValue()));
    }
    else if (onCancel)
    {
        onCancel();
    }
}

void ControllerValueEditor::dismissBox()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}
