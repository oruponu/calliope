#include "ui/pianoroll/ProgramBrowser.h"
#include "model/GmPrograms.h"
#include "ui/theme/Theme.h"

namespace
{
constexpr int rowHeight = 24;
constexpr int familyColumnWidth = 168;
constexpr int programColumnWidth = 264;
constexpr int bankValueWidth = 56;
constexpr int textInset = 8;

juce::Rectangle<int> bankValueBounds(juce::Rectangle<int> bounds)
{
    return bounds.removeFromRight(bankValueWidth).reduced(4, 3);
}
} // namespace

ProgramBrowser::ProgramBrowser(const ProgramVoice& voice) : state(ProgramBrowserState::at(voice))
{
    using namespace calliope::theme;

    struct BankField
    {
        WheelLabel* label;
        ProgramBrowserState::Column column;
    };
    for (auto field : {BankField{&bankMsbLabel, ProgramBrowserState::Column::BankMsb},
                       BankField{&bankLsbLabel, ProgramBrowserState::Column::BankLsb}})
    {
        auto* label = field.label;
        const auto column = field.column;
        addAndMakeVisible(label);
        label->setFont(font::sans(font::sizeMD));
        label->setColour(juce::Label::textColourId, text::t1);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setBorderSize(juce::BorderSize<int>(0));
        label->setMinimumHorizontalScale(1.0f);
        label->setEditable(true);
        label->setWantsKeyboardFocus(false);
        label->onEditorShow = [this, label, column]()
        {
            state.column = column;
            repaint();
            if (auto* editor = label->getCurrentTextEditor())
            {
                editor->setInputRestrictions(3, "0123456789");
                editor->setJustification(juce::Justification::centredLeft);
                editor->setEscapeAndReturnKeysConsumed(false);
                editor->selectAll();
            }
        };
        label->onEditorHide = [this]()
        {
            juce::MessageManager::callAsync(
                [safe = juce::Component::SafePointer<ProgramBrowser>(this)]()
                {
                    if (safe != nullptr && !safe->bankMsbLabel.isBeingEdited() && !safe->bankLsbLabel.isBeingEdited())
                        safe->grabKeyboardFocus();
                });
        };
        label->onTextChange = [this, label, column]() { setBankValueFromLabel(*label, column); };
        label->onWheel = [this, column](int direction)
        {
            const int value = column == ProgramBrowserState::Column::BankMsb ? state.bank.msb : state.bank.lsb;
            state.setBankValue(column, value + direction);
            refreshBankLabels();
            repaint();
        };
    }

    refreshBankLabels();
    setWantsKeyboardFocus(true);
    setSize(familyColumnWidth + programColumnWidth, rowHeight * (GmPrograms::familyCount + 1));
}

ProgramBrowser::~ProgramBrowser()
{
    finalize(false);
}

void ProgramBrowser::abandon()
{
    finalized = true;
    onCommit = nullptr;
    onCancel = nullptr;
}

void ProgramBrowser::paint(juce::Graphics& g)
{
    using namespace calliope::theme;

    g.fillAll(surface::surface);
    g.setFont(font::sans(font::sizeMD));

    for (int family = 0; family < GmPrograms::familyCount; ++family)
    {
        const auto row = familyBounds.withY(familyBounds.getY() + family * rowHeight).withHeight(rowHeight);
        const bool selected = family == state.family;
        if (selected)
        {
            g.setColour(accent::soft);
            g.fillRect(row);
        }
        g.setColour(selected ? (state.column == ProgramBrowserState::Column::Families ? accent::strong : accent::dim)
                             : text::t2);
        g.drawText(GmPrograms::familyName(family), row.reduced(textInset, 0), juce::Justification::centredLeft, false);
    }

    for (int rowIndex = 0; rowIndex < GmPrograms::programsPerFamily; ++rowIndex)
    {
        const int program = state.family * GmPrograms::programsPerFamily + rowIndex;
        const auto row = programBounds.withY(programBounds.getY() + rowIndex * rowHeight).withHeight(rowHeight);
        const bool selected = program == state.program;
        if (selected)
        {
            g.setColour(accent::soft);
            g.fillRect(row);
        }
        g.setColour(selected ? (state.column == ProgramBrowserState::Column::Programs ? accent::strong : accent::dim)
                             : text::t1);
        const auto label = juce::String(program) + " " + GmPrograms::programName(program);
        g.drawText(label, row.reduced(textInset, 0), juce::Justification::centredLeft, false);
    }

    const auto drawBankField =
        [&](juce::Rectangle<int> bounds, const juce::String& label, ProgramBrowserState::Column column)
    {
        const auto valueBounds = bankValueBounds(bounds);
        g.setColour(text::t2);
        g.drawText(label, bounds.withRight(valueBounds.getX()).reduced(textInset, 0), juce::Justification::centredLeft,
                   false);
        const bool focused = state.column == column;
        g.setColour(focused ? accent::strong : border::normal);
        g.drawRect(valueBounds, 1);
    };
    drawBankField(bankMsbBounds, "Bank MSB", ProgramBrowserState::Column::BankMsb);
    drawBankField(bankLsbBounds, "Bank LSB", ProgramBrowserState::Column::BankLsb);

    g.setColour(border::normal);
    g.drawVerticalLine(programBounds.getX(), 0.0f, static_cast<float>(getHeight()));
    g.drawHorizontalLine(bankMsbBounds.getY(), 0.0f, static_cast<float>(getWidth()));
}

void ProgramBrowser::resized()
{
    auto area = getLocalBounds();
    auto bankArea = area.removeFromBottom(rowHeight);
    bankMsbBounds = bankArea.removeFromLeft(familyColumnWidth);
    bankLsbBounds = bankArea;
    familyBounds = area.removeFromLeft(familyColumnWidth);
    programBounds = area;
    bankMsbLabel.setBounds(bankValueBounds(bankMsbBounds));
    bankLsbLabel.setBounds(bankValueBounds(bankLsbBounds));
}

void ProgramBrowser::mouseDown(const juce::MouseEvent& event)
{
    const auto position = event.getPosition();
    if (familyBounds.contains(position))
    {
        const int family = (position.y - familyBounds.getY()) / rowHeight;
        if (family >= 0 && family < GmPrograms::familyCount)
        {
            state.chooseFamily(family);
            hideBankEditors();
            repaint();
            grabKeyboardFocus();
        }
        return;
    }

    if (programBounds.contains(position))
    {
        const int row = (position.y - programBounds.getY()) / rowHeight;
        if (row >= 0 && row < GmPrograms::programsPerFamily)
        {
            state.program = state.family * GmPrograms::programsPerFamily + row;
            finalize(true);
            dismissBox();
        }
        return;
    }

    if (bankMsbBounds.contains(position) || bankLsbBounds.contains(position))
    {
        state.column = bankMsbBounds.contains(position) ? ProgramBrowserState::Column::BankMsb
                                                        : ProgramBrowserState::Column::BankLsb;
        showEditorForColumn();
    }
}

bool ProgramBrowser::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::upKey)
        state.moveUp();
    else if (key == juce::KeyPress::downKey)
        state.moveDown();
    else if (key == juce::KeyPress::leftKey)
    {
        state.moveLeft();
        showEditorForColumn();
    }
    else if (key == juce::KeyPress::rightKey)
    {
        state.moveRight();
        showEditorForColumn();
    }
    else if (key.getKeyCode() == juce::KeyPress::tabKey)
    {
        if (key.getModifiers().isShiftDown())
            state.focusPrevious();
        else
            state.focusNext();
        showEditorForColumn();
    }
    else if (key == juce::KeyPress::returnKey)
    {
        finalize(true);
        dismissBox();
        return true;
    }
    else if (key == juce::KeyPress::escapeKey)
    {
        finalize(false);
        dismissBox();
        return true;
    }
    else
        return false;

    repaint();
    return true;
}

void ProgramBrowser::refreshBankLabels()
{
    bankMsbLabel.setText(juce::String(state.bank.msb), juce::dontSendNotification);
    bankLsbLabel.setText(juce::String(state.bank.lsb), juce::dontSendNotification);
}

void ProgramBrowser::setBankValueFromLabel(WheelLabel& label, ProgramBrowserState::Column column)
{
    const auto text = label.getText(true);
    state.setBankValue(column, text.isEmpty() ? 0 : text.getIntValue());
    const int value = column == ProgramBrowserState::Column::BankMsb ? state.bank.msb : state.bank.lsb;
    label.setText(juce::String(value), juce::dontSendNotification);
    repaint();
}

void ProgramBrowser::showEditorForColumn()
{
    WheelLabel* target = nullptr;
    if (state.column == ProgramBrowserState::Column::BankMsb)
        target = &bankMsbLabel;
    else if (state.column == ProgramBrowserState::Column::BankLsb)
        target = &bankLsbLabel;

    if (target != &bankMsbLabel)
        bankMsbLabel.hideEditor(false);
    if (target != &bankLsbLabel)
        bankLsbLabel.hideEditor(false);
    if (target != nullptr)
        target->showEditor();
    repaint();
}

void ProgramBrowser::hideBankEditors()
{
    bankMsbLabel.hideEditor(false);
    bankLsbLabel.hideEditor(false);
}

void ProgramBrowser::parentHierarchyChanged()
{
    if (!isShowing() || !initialFocusPending)
        return;

    initialFocusPending = false;
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<ProgramBrowser>(this)]()
        {
            if (safe != nullptr)
                safe->grabKeyboardFocus();
        });
}

void ProgramBrowser::finalize(bool commit)
{
    if (finalized)
        return;
    if (commit)
    {
        setBankValueFromLabel(bankMsbLabel, ProgramBrowserState::Column::BankMsb);
        setBankValueFromLabel(bankLsbLabel, ProgramBrowserState::Column::BankLsb);
    }
    finalized = true;
    if (commit)
    {
        if (onCommit)
            onCommit(state.voice());
    }
    else if (onCancel)
    {
        onCancel();
    }
}

void ProgramBrowser::dismissBox()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}
