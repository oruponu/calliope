#include "TrackListComponent.h"
#include "TrackColours.h"

namespace
{
constexpr int kStripeX = 10;
constexpr int kContentLeft = 18;
constexpr int kButtonSize = 20;
constexpr int kButtonGap = 2;
constexpr int kRightPad = 10;
constexpr int kRow1Y = 6;
constexpr int kRow2Y = 28;
constexpr int kRow2H = 16;
constexpr int kOutputWidth = 116;
constexpr int kChannelWidth = 52;
constexpr int kComboGap = 4;
} // namespace

void TrackListComponent::setSequence(MidiSequence* seq)
{
    cancelNameEdit();
    sequence = seq;
    if (seq && seq->getNumTracks() > 0)
    {
        activeTrackIndex = 0;
        anchorTrackIndex = 0;
        selectedTrackIndices.clear();
        for (int i = 0; i < seq->getNumTracks(); ++i)
            selectedTrackIndices.insert(i);
    }
    else
    {
        activeTrackIndex = -1;
        anchorTrackIndex = -1;
        selectedTrackIndices.clear();
    }
    updateSize();
    repaint();
}

void TrackListComponent::refresh()
{
    cancelNameEdit();
    updateSize();
    repaint();
}

void TrackListComponent::updateSize()
{
    int numTracks = sequence ? sequence->getNumTracks() : 0;
    int requiredHeight = numTracks * trackRowHeight + addButtonRowHeight;
    setSize(getWidth(), juce::jmax(requiredHeight, getParentHeight()));
}

int TrackListComponent::getActiveTrackIndex() const
{
    return activeTrackIndex;
}

void TrackListComponent::setActiveTrackIndex(int index)
{
    if (!sequence || index < 0 || index >= sequence->getNumTracks())
        return;
    activeTrackIndex = index;
    anchorTrackIndex = index;
    selectedTrackIndices = {index};
    repaint();
    notifySelectionChanged();
}

const std::set<int>& TrackListComponent::getSelectedTrackIndices() const
{
    return selectedTrackIndices;
}

bool TrackListComponent::isTrackSelected(int index) const
{
    return selectedTrackIndices.contains(index);
}

void TrackListComponent::setSelectedTrackIndices(const std::set<int>& indices)
{
    selectedTrackIndices = indices;
    repaint();
}

void TrackListComponent::notifySelectionChanged()
{
    if (onTrackSelected)
        onTrackSelected(activeTrackIndex, selectedTrackIndices);
}

void TrackListComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(surface::bg2);

    if (!sequence)
        return;

    for (int i = 0; i < sequence->getNumTracks(); ++i)
    {
        const auto& track = sequence->getTrack(i);
        int y = i * trackRowHeight;

        auto highlightBounds = juce::Rectangle<int>(4, y + 2, getWidth() - 8, trackRowHeight - 4).toFloat();
        if (i == activeTrackIndex)
        {
            g.setColour(accent::soft);
            g.fillRoundedRectangle(highlightBounds, radius::r2);
        }
        else if (selectedTrackIndices.contains(i))
        {
            g.setColour(surface::hover);
            g.fillRoundedRectangle(highlightBounds, radius::r2);
        }

        g.setColour(TrackColours::getColour(i));
        g.fillRoundedRectangle(juce::Rectangle<float>(kStripeX, y + 10.0f, 3.0f, trackRowHeight - 20.0f), 1.5f);

        if (i != editingRow)
        {
            g.setColour(text::t1);
            g.setFont(font::sans(font::sizeLG));
            juce::String trackLabel =
                track.getName().empty() ? "Track " + juce::String(i + 1) : juce::String(track.getName());
            g.drawText(trackLabel, getNameLabelBounds(i), juce::Justification::centredLeft);
        }

        auto pluginBounds = getPluginLabelBounds(i);
        auto channelBounds = getChannelLabelBounds(i);

        juce::String pluginName = pluginNameForTrack ? pluginNameForTrack(i) : juce::String{};
        auto destination = track.getOutputDestination();
        juce::String labelText;
        bool labelActive = false;
        switch (destination)
        {
        case MidiTrack::OutputDestination::Plugin:
        {
            int targetIdx = track.getRouteTargetTrackIndex();
            if (targetIdx < 0 || targetIdx >= sequence->getNumTracks() || targetIdx == i)
            {
                labelText = juce::String::fromUTF8("\xe2\x96\xb6 ") +
                            (pluginName.isEmpty() ? juce::String("(no plugin)") : pluginName);
                labelActive = !pluginName.isEmpty();
            }
            else
            {
                const auto& targetTrack = sequence->getTrack(targetIdx);
                juce::String targetPluginName = pluginNameForTrack ? pluginNameForTrack(targetIdx) : juce::String{};
                if (!targetPluginName.isEmpty())
                {
                    juce::String targetLabel = targetTrack.getName().empty() ? "Track " + juce::String(targetIdx + 1)
                                                                             : juce::String(targetTrack.getName());
                    labelText = juce::String::fromUTF8("\xe2\x86\x92 ") + targetLabel;
                    labelActive = true;
                }
                else
                {
                    labelText = juce::String::fromUTF8("\xe2\x86\x92 (invalid)");
                    labelActive = false;
                }
            }
            break;
        }
        case MidiTrack::OutputDestination::MidiDevice:
            labelText = juce::String::fromUTF8("\xe3\x80\xb0 MIDI");
            labelActive = true;
            break;
        case MidiTrack::OutputDestination::None:
            labelText = juce::String::fromUTF8("\xe2\x8a\x98 Off");
            labelActive = false;
            break;
        }
        auto drawCombo =
            [&](juce::Rectangle<int> b, const juce::String& txt, const juce::Font& fnt, juce::Colour txtColour)
        {
            auto rect = b.toFloat();
            g.setColour(surface::surface2);
            g.fillRoundedRectangle(rect, radius::r1);
            g.setColour(border::normal);
            g.drawRoundedRectangle(rect.reduced(0.5f), radius::r1, 1.0f);

            float cx = rect.getRight() - 9.0f;
            float cy = rect.getCentreY();
            juce::Path chevron;
            chevron.startNewSubPath(cx - 3.0f, cy - 1.5f);
            chevron.lineTo(cx, cy + 2.0f);
            chevron.lineTo(cx + 3.0f, cy - 1.5f);
            g.setColour(text::t3);
            g.strokePath(chevron, juce::PathStrokeType(1.0f));

            g.setColour(txtColour);
            g.setFont(fnt);
            g.drawText(txt, b.withTrimmedLeft(6).withTrimmedRight(14), juce::Justification::centredLeft);
        };

        drawCombo(pluginBounds, labelText, font::sans(font::sizeXS), labelActive ? text::t2 : text::t3);
        drawCombo(channelBounds, "Ch " + juce::String(track.getChannel()).paddedLeft('0', 2), font::sans(font::sizeXS),
                  text::t2);

        auto drawToggle =
            [&](juce::Rectangle<int> b, const juce::String& label, bool on, juce::Colour onFill, juce::Colour onText)
        {
            auto rect = b.toFloat();
            if (on)
            {
                g.setColour(onFill);
                g.fillRoundedRectangle(rect, radius::r1);
            }
            else
            {
                g.setColour(border::normal);
                g.drawRoundedRectangle(rect.reduced(0.5f), radius::r1, 1.0f);
            }
            g.setColour(on ? onText : text::t3);
            g.setFont(font::sans(font::sizeXS).boldened());
            g.drawText(label, b, juce::Justification::centred);
        };

        bool hasPlugin = !pluginName.isEmpty();
        drawToggle(getEditorButtonBounds(i), "e", hasPlugin, accent::soft, accent::strong);
        drawToggle(getMuteButtonBounds(i), "M", track.isMuted(), status::danger, text::t1);
        drawToggle(getSoloButtonBounds(i), "S", track.isSolo(), status::warn, surface::bg);
    }

    auto addBounds = getAddButtonBounds().reduced(8, 8).toFloat();
    juce::Path dashed;
    dashed.addRoundedRectangle(addBounds, radius::r2);
    juce::Path stroked;
    const float dashes[] = {4.0f, 3.0f};
    juce::PathStrokeType(1.0f).createDashedStroke(stroked, dashed, dashes, 2);
    g.setColour(border::strong);
    g.fillPath(stroked);
    g.setColour(text::t3);
    g.setFont(font::sans(font::sizeSM));
    g.drawText("+ Add Track", getAddButtonBounds(), juce::Justification::centred);
}

void TrackListComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence)
        return;

    if (nameEditor != nullptr)
        commitNameEdit();

    if (e.mods.isPopupMenu())
    {
        int row = getRowIndexAt(e.y);
        bool onTrack = row >= 0 && row < sequence->getNumTracks();
        auto screenPos = e.getScreenPosition();
        juce::PopupMenu menu;
        if (onTrack)
        {
            bool canRemove = sequence->getNumTracks() > 1;
            menu.addItem("Delete Track", canRemove, false,
                         [this, row]()
                         {
                             if (onRemoveTrackRequested)
                                 onRemoveTrackRequested(row);
                         });
        }
        else
        {
            menu.addItem("Add Track", true, false,
                         [this]()
                         {
                             if (onAddTrackRequested)
                                 onAddTrackRequested();
                         });
        }
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}));
        return;
    }

    if (getAddButtonBounds().contains(e.x, e.y))
    {
        if (onAddTrackRequested)
            onAddTrackRequested();
        return;
    }

    int row = getRowIndexAt(e.y);
    if (row < 0 || row >= sequence->getNumTracks())
        return;

    auto& track = sequence->getTrack(row);

    if (getMuteButtonBounds(row).contains(e.x, e.y))
    {
        bool newMuted = !track.isMuted();
        if (selectedTrackIndices.contains(row))
        {
            for (int idx : selectedTrackIndices)
                sequence->getTrack(idx).setMuted(newMuted);
        }
        else
        {
            track.setMuted(newMuted);
        }
        repaint();
        if (onMuteSoloChanged)
            onMuteSoloChanged();
        return;
    }

    if (getSoloButtonBounds(row).contains(e.x, e.y))
    {
        bool newSolo = !track.isSolo();
        if (selectedTrackIndices.contains(row))
        {
            for (int idx : selectedTrackIndices)
                sequence->getTrack(idx).setSolo(newSolo);
        }
        else
        {
            track.setSolo(newSolo);
        }
        repaint();
        if (onMuteSoloChanged)
            onMuteSoloChanged();
        return;
    }

    if (getEditorButtonBounds(row).contains(e.x, e.y))
    {
        if (onEditorButtonClicked)
            onEditorButtonClicked(row);
        return;
    }

    if (getPluginLabelBounds(row).contains(e.x, e.y))
    {
        if (onPluginLabelClicked)
            onPluginLabelClicked(row);
        return;
    }

    if (getChannelLabelBounds(row).contains(e.x, e.y))
    {
        if (onChannelLabelClicked)
            onChannelLabelClicked(row);
        return;
    }

    if (e.mods.isCommandDown())
    {
        if (selectedTrackIndices.contains(row))
        {
            if (selectedTrackIndices.size() > 1)
            {
                selectedTrackIndices.erase(row);
                if (activeTrackIndex == row)
                    activeTrackIndex = *selectedTrackIndices.rbegin();
            }
        }
        else
        {
            selectedTrackIndices.insert(row);
        }
        activeTrackIndex = row;
        anchorTrackIndex = row;
    }
    else if (e.mods.isShiftDown())
    {
        selectedTrackIndices.clear();
        int lo = std::min(anchorTrackIndex, row);
        int hi = std::max(anchorTrackIndex, row);
        for (int i = lo; i <= hi; ++i)
            selectedTrackIndices.insert(i);
        activeTrackIndex = row;
    }
    else
    {
        activeTrackIndex = row;
        anchorTrackIndex = row;
        selectedTrackIndices = {row};
    }
    repaint();
    notifySelectionChanged();
}

void TrackListComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isPopupMenu())
        return;

    int row = getRowIndexAt(e.y);
    if (row < 0 || row >= sequence->getNumTracks())
        return;

    if (getNameLabelBounds(row).contains(e.x, e.y))
        beginEditingName(row);
}

void TrackListComponent::beginEditingName(int rowIndex)
{
    if (!sequence || rowIndex < 0 || rowIndex >= sequence->getNumTracks())
        return;

    cancelNameEdit();

    const auto& track = sequence->getTrack(rowIndex);
    juce::String initialText =
        track.getName().empty() ? "Track " + juce::String(rowIndex + 1) : juce::String(track.getName());

    editingRow = rowIndex;
    nameEditor = std::make_unique<juce::TextEditor>();
    nameEditor->setBounds(getNameLabelBounds(rowIndex));
    nameEditor->setFont(calliope::theme::font::sans(calliope::theme::font::sizeLG));
    nameEditor->setText(initialText, juce::dontSendNotification);
    nameEditor->setSelectAllWhenFocused(true);
    nameEditor->onReturnKey = [this]() { commitNameEdit(); };
    nameEditor->onFocusLost = [this]() { commitNameEdit(); };
    nameEditor->onEscapeKey = [this]() { cancelNameEdit(); };
    addAndMakeVisible(*nameEditor);
    nameEditor->grabKeyboardFocus();
    nameEditor->selectAll();
    repaint();
}

void TrackListComponent::commitNameEdit()
{
    if (!nameEditor || editingRow < 0)
        return;

    int row = editingRow;
    juce::String newName = nameEditor->getText().trim();

    editingRow = -1;
    auto editor = std::move(nameEditor);
    editor.reset();

    if (sequence && row < sequence->getNumTracks())
    {
        juce::String currentName = juce::String(sequence->getTrack(row).getName());
        if (newName != currentName && onTrackRenamed)
            onTrackRenamed(row, newName);
    }
    repaint();
}

void TrackListComponent::cancelNameEdit()
{
    if (!nameEditor)
        return;
    editingRow = -1;
    auto editor = std::move(nameEditor);
    editor.reset();
    repaint();
}

int TrackListComponent::getRowIndexAt(int y) const
{
    return y / trackRowHeight;
}

juce::Rectangle<int> TrackListComponent::getEditorButtonBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {kContentLeft, y + kRow1Y, kButtonSize, kButtonSize};
}

juce::Rectangle<int> TrackListComponent::getMuteButtonBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {kContentLeft + (kButtonSize + kButtonGap), y + kRow1Y, kButtonSize, kButtonSize};
}

juce::Rectangle<int> TrackListComponent::getSoloButtonBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {kContentLeft + 2 * (kButtonSize + kButtonGap), y + kRow1Y, kButtonSize, kButtonSize};
}

juce::Rectangle<int> TrackListComponent::getNameLabelBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    int x = getSoloButtonBounds(rowIndex).getRight() + 8;
    int width = juce::jmax(0, getWidth() - kRightPad - x);
    return {x, y + kRow1Y, width, kButtonSize};
}

juce::Rectangle<int> TrackListComponent::getPluginLabelBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    int maxRight = getWidth() - kRightPad;
    int width = kOutputWidth;
    if (kContentLeft + width + kComboGap + kChannelWidth > maxRight)
        width = juce::jmax(30, maxRight - kContentLeft - kComboGap - kChannelWidth);
    return {kContentLeft, y + kRow2Y, width, kRow2H};
}

juce::Rectangle<int> TrackListComponent::getChannelLabelBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    int x = getPluginLabelBounds(rowIndex).getRight() + kComboGap;
    return {x, y + kRow2Y, kChannelWidth, kRow2H};
}

juce::Rectangle<int> TrackListComponent::getAddButtonBounds() const
{
    int numTracks = sequence ? sequence->getNumTracks() : 0;
    int y = numTracks * trackRowHeight;
    return {0, y, getWidth(), addButtonRowHeight};
}
