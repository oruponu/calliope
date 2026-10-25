#include "ui/pianoroll/ProgramBrowser.h"
#include "model/GmPrograms.h"
#include "ui/theme/Theme.h"

namespace
{
constexpr int rowHeight = 24;
constexpr int familyColumnWidth = 168;
constexpr int programColumnWidth = 264;
constexpr int textInset = 8;
} // namespace

ProgramBrowser::ProgramBrowser(int program) : state(ProgramBrowserState::at(program))
{
    setWantsKeyboardFocus(true);
    setSize(familyColumnWidth + programColumnWidth, rowHeight * GmPrograms::familyCount);
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

    g.setColour(border::normal);
    g.drawVerticalLine(programBounds.getX(), 0.0f, static_cast<float>(getHeight()));
}

void ProgramBrowser::resized()
{
    auto area = getLocalBounds();
    familyBounds = area.removeFromLeft(familyColumnWidth);
    programBounds = area;
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
    }
}

bool ProgramBrowser::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::upKey)
        state.moveUp();
    else if (key == juce::KeyPress::downKey)
        state.moveDown();
    else if (key == juce::KeyPress::leftKey)
        state.moveLeft();
    else if (key == juce::KeyPress::rightKey)
        state.moveRight();
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
    {
        return false;
    }

    repaint();
    return true;
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
    finalized = true;
    if (commit)
    {
        if (onCommit)
            onCommit(state.program);
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
