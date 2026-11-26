#include "ui/pianoroll/PianoRollToolBar.h"
#include "settings/AppProperties.h"
#include "ui/pianoroll/GridItemIds.h"
#include "ui/theme/Theme.h"

PianoRollToolBar::PianoRollToolBar()
{
    addAndMakeVisible(editToolButton);
    editToolButton.onClick = [this]() { onToolClicked(PianoRollComponent::EditMode::Edit); };

    addAndMakeVisible(selectToolButton);
    selectToolButton.setActive(true);
    selectToolButton.onClick = [this]() { onToolClicked(PianoRollComponent::EditMode::Select); };

    addAndMakeVisible(snapButton);
    const bool snapEnabled = getAppProperties().getUserSettings()->getBoolValue("snapEnabled", true);
    snapButton.setActive(snapEnabled);
    snapButton.onClick = [this]() { toggleSnap(); };

    addAndMakeVisible(quantizeComboBox);
    quantizeComboBox.addItem("1/1", 1);
    quantizeComboBox.addItem("1/2", 2);
    quantizeComboBox.addItem("1/4", 4);
    quantizeComboBox.addItem("1/8", 8);
    quantizeComboBox.addItem("1/16", 16);
    quantizeComboBox.addItem("1/32", 32);
    quantizeComboBox.addSeparator();
    for (int denominator : {2, 4, 8, 16, 32})
        quantizeComboBox.addItem("1/" + juce::String(denominator) + "T", GridItemIds::forDivision({denominator, true}));
    quantizeComboBox.setSelectedId(16, juce::dontSendNotification);
    quantizeComboBox.onChange = [this]() { onGridDivisionChanged(getGridDivision()); };

    selectToolButton.setWantsKeyboardFocus(true);
    editToolButton.setWantsKeyboardFocus(true);
    snapButton.setWantsKeyboardFocus(true);
    quantizeComboBox.setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(false);
}

void PianoRollToolBar::setActiveTool(PianoRollComponent::EditMode mode)
{
    editToolButton.setActive(mode == PianoRollComponent::EditMode::Edit);
    selectToolButton.setActive(mode == PianoRollComponent::EditMode::Select);
}

bool PianoRollToolBar::isSnapEnabled() const
{
    return snapButton.isActive();
}

void PianoRollToolBar::toggleSnap()
{
    const bool enabled = !snapButton.isActive();
    snapButton.setActive(enabled);
    getAppProperties().getUserSettings()->setValue("snapEnabled", enabled);
    onSnapChanged(enabled);
}

GridDivision PianoRollToolBar::getGridDivision() const
{
    return GridItemIds::toDivision(quantizeComboBox.getSelectedId());
}

bool PianoRollToolBar::contains(const juce::Component* focused) const
{
    auto containsFocused = [focused](const juce::Component& component)
    { return &component == focused || component.isParentOf(focused); };
    return containsFocused(editToolButton) || containsFocused(selectToolButton) || containsFocused(snapButton) ||
           containsFocused(quantizeComboBox);
}

void PianoRollToolBar::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto bounds = getLocalBounds();
    g.setColour(surface::surface2);
    g.fillRect(bounds);
    g.setColour(border::strong);
    g.drawHorizontalLine(bounds.getBottom() - 1, static_cast<float>(bounds.getX()),
                         static_cast<float>(bounds.getRight()));
    g.drawVerticalLine(separatorX, static_cast<float>(bounds.getY() + 8), static_cast<float>(bounds.getBottom() - 8));
}

void PianoRollToolBar::resized()
{
    const int btnSize = 28;
    const int pad = 4;
    auto toolBarArea = getLocalBounds();
    auto toolBtnArea =
        toolBarArea.withTrimmedLeft(pad * 2).withSizeKeepingCentre(toolBarArea.getWidth() - pad * 2, btnSize);
    selectToolButton.setBounds(toolBtnArea.removeFromLeft(btnSize));
    toolBtnArea.removeFromLeft(pad);
    editToolButton.setBounds(toolBtnArea.removeFromLeft(btnSize));
    toolBtnArea.removeFromLeft(pad * 2);
    separatorX = toolBtnArea.getX();
    toolBtnArea.removeFromLeft(pad * 2);
    snapButton.setBounds(toolBtnArea.removeFromLeft(btnSize));
    toolBtnArea.removeFromLeft(pad);
    quantizeComboBox.setBounds(toolBtnArea.removeFromLeft(70).withSizeKeepingCentre(70, 26));
}

void PianoRollToolBar::mouseDown(const juce::MouseEvent&)
{
    onBackgroundClicked();
}
