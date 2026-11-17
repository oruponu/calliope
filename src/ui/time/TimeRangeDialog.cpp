#include "ui/time/TimeRangeDialog.h"
#include "ui/eventlist/EventListPosition.h"
#include "ui/theme/Theme.h"
#include "ui/time/TimeRangeInput.h"
#include <utility>

namespace
{
constexpr std::array maxLengths{4, 2, 5};
constexpr std::array paddedLengths{3, 2, 4};

juce::String fieldText(int value, size_t field)
{
    return juce::String(value).paddedLeft('0', paddedLengths[field]);
}
} // namespace

void TimeRangeDialog::launch(const TimeRangeDialogTexts& texts, const TimelineMap& timeline, BarBeatTick initialStart,
                             std::function<std::optional<juce::String>(int start, int end)> apply,
                             juce::Component& owner)
{
    auto* content = new TimeRangeDialog(texts, timeline, initialStart, std::move(apply));

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(content);
    options.dialogTitle = texts.title;
    options.dialogBackgroundColour = calliope::theme::surface::bg2;
    options.componentToCentreAround = &owner;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    auto* window = options.launchAsync();

    // LaunchOptions fits the window around JUCE's own title bar before switching to the native one, which stretches the
    // content by that bar's height, so the window is fitted to the content again.
    content->setSize(width, height);
    window->centreAroundComponent(&owner, window->getWidth(), window->getHeight());

    // The window takes the focus when it goes modal, so the field takes it back once that has settled.
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<TimeRangeDialog>(content)]
        {
            if (safe == nullptr)
                return;
            safe->startFields[0].grabKeyboardFocus();
            safe->startFields[0].selectAll();
        });
}

TimeRangeDialog::TimeRangeDialog(const TimeRangeDialogTexts& texts, const TimelineMap& initialTimeline,
                                 BarBeatTick initialStart, std::function<std::optional<juce::String>(int, int)> onApply)
    : timeline(initialTimeline), apply(std::move(onApply)), applyButton(texts.applyButton)
{
    using namespace calliope::theme;

    for (auto* label : {&startLabel, &endLabel})
    {
        label->setFont(font::sans(font::sizeMD));
        label->setColour(juce::Label::textColourId, text::t2);
        addAndMakeVisible(*label);
    }

    setUpFields(startFields, initialStart);
    setUpFields(endFields, TimeRangeInput::initialEnd(initialStart, timeline));

    errorLabel.setFont(font::sans(font::sizeSM));
    errorLabel.setColour(juce::Label::textColourId, text::t2);
    errorLabel.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(errorLabel);

    for (auto* button : {&applyButton, &cancelButton})
    {
        button->setWantsKeyboardFocus(false);
        addAndMakeVisible(*button);
    }
    applyButton.onClick = [this] { applyAndClose(); };
    cancelButton.onClick = [this] { close(); };

    onFieldsChanged();
    setSize(width, height);
}

void TimeRangeDialog::setUpFields(Fields& fields, const BarBeatTick& value)
{
    using namespace calliope::theme;

    for (size_t i = 0; i < fields.size(); ++i)
    {
        auto& editor = fields[i];
        editor.setFont(font::mono(font::sizeMD));
        editor.setJustification(juce::Justification::centred);
        editor.setInputRestrictions(maxLengths[i], "0123456789");
        editor.setSelectAllWhenFocused(true);
        editor.onTextChange = [this] { onFieldsChanged(); };
        editor.onFocusLost = [&editor, i]
        {
            if (!editor.isEmpty())
                editor.setText(fieldText(editor.getText().getIntValue(), i), false);
            // A field left unchanged keeps its selection otherwise, since setText ignores the same text.
            editor.moveCaretToEnd();
        };
        editor.onReturnKey = [this] { applyAndClose(); };
        editor.onEscapeKey = [this] { close(); };
        addAndMakeVisible(editor);
    }
    showPosition(fields, value);
}

void TimeRangeDialog::showPosition(Fields& fields, const BarBeatTick& value)
{
    const std::array values{value.bar, value.beat, value.tick};
    for (size_t i = 0; i < fields.size(); ++i)
        fields[i].setText(fieldText(values[i], i), false);
}

void TimeRangeDialog::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(surface::bg2);
    g.setColour(text::t2);
    g.setFont(font::sans(font::sizeMD));
    for (const auto* fields : {&startFields, &endFields})
        for (size_t i = 0; i + 1 < fields->size(); ++i)
            g.drawText(".", (*fields)[i].getRight(), (*fields)[i].getY(), dotWidth, (*fields)[i].getHeight(),
                       juce::Justification::centred);
}

void TimeRangeDialog::layoutFields(Fields& fields, juce::Rectangle<int> row)
{
    fields[0].setBounds(row.removeFromLeft(barWidth));
    row.removeFromLeft(dotWidth);
    fields[1].setBounds(row.removeFromLeft(beatWidth));
    row.removeFromLeft(dotWidth);
    fields[2].setBounds(row.removeFromLeft(tickWidth));
}

void TimeRangeDialog::resized()
{
    auto area = getLocalBounds().reduced(padding);

    auto startRow = area.removeFromTop(rowHeight);
    startLabel.setBounds(startRow.removeFromLeft(labelWidth));
    startRow.removeFromLeft(gap);
    layoutFields(startFields, startRow);

    area.removeFromTop(rowGap);
    auto endRow = area.removeFromTop(rowHeight);
    endLabel.setBounds(endRow.removeFromLeft(labelWidth));
    endRow.removeFromLeft(gap);
    layoutFields(endFields, endRow);

    if (errorLabel.isVisible())
    {
        area.removeFromTop(errorGap);
        errorLabel.setBounds(area.removeFromTop(errorHeight));
    }

    area.removeFromTop(buttonRowGap);
    auto buttonRow = area.removeFromTop(rowHeight);
    cancelButton.setBounds(buttonRow.removeFromRight(buttonWidth));
    buttonRow.removeFromRight(gap);
    applyButton.setBounds(buttonRow.removeFromRight(buttonWidth));
}

std::optional<BarBeatTick> TimeRangeDialog::readPosition(const Fields& fields) const
{
    return TimeRangeInput::parse(fields[0].getText().toStdString(), fields[1].getText().toStdString(),
                                 fields[2].getText().toStdString());
}

std::optional<std::pair<int, int>> TimeRangeDialog::readRange() const
{
    const auto start = readPosition(startFields);
    const auto end = readPosition(endFields);
    if (!start || !end)
        return std::nullopt;
    const int startTick = EventListPosition::toTick(*start, timeline);
    const int endTick = EventListPosition::toTick(*end, timeline);
    if (endTick <= startTick)
        return std::nullopt;
    return std::pair{startTick, endTick};
}

void TimeRangeDialog::onFieldsChanged()
{
    showError({});
    applyButton.setEnabled(readRange().has_value());
}

// The dialog grows to make room for the reason, so no blank space waits for it above the buttons.
void TimeRangeDialog::showError(const juce::String& reason)
{
    errorLabel.setText(reason, juce::dontSendNotification);
    errorLabel.setVisible(reason.isNotEmpty());
    setSize(width, reason.isEmpty() ? height : heightWithError);
}

void TimeRangeDialog::showKeptInside(Fields& fields)
{
    const auto position = readPosition(fields);
    if (!position)
        return;
    showPosition(fields, TimeRangeInput::keptInside(*position, timeline));
}

void TimeRangeDialog::applyAndClose()
{
    if (!readRange())
        return;
    showKeptInside(startFields);
    showKeptInside(endFields);
    const auto range = readRange();
    if (!range)
        return;
    if (const auto reason = apply(range->first, range->second))
        showError(*reason);
    else
        close();
}

void TimeRangeDialog::close()
{
    if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
        window->exitModalState(0);
}
