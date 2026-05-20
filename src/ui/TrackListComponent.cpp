#include "TrackListComponent.h"
#include "TrackColours.h"

void TrackListComponent::setSequence(MidiSequence* seq)
{
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
    return selectedTrackIndices.count(index) > 0;
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
    g.fillAll(juce::Colour(30, 30, 38));

    if (!sequence)
        return;

    for (int i = 0; i < sequence->getNumTracks(); ++i)
    {
        const auto& track = sequence->getTrack(i);
        int y = i * trackRowHeight;

        auto rowBounds = juce::Rectangle<int>(0, y, getWidth(), trackRowHeight);

        if (i == activeTrackIndex)
            g.setColour(juce::Colour(55, 55, 70));
        else if (selectedTrackIndices.count(i) > 0)
            g.setColour(juce::Colour(48, 48, 60));
        else
            g.setColour(juce::Colour(40, 40, 48));
        g.fillRect(rowBounds);

        g.setColour(TrackColours::getColour(i));
        g.fillRect(0, y, 4, trackRowHeight);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(13.0f)).boldened());
        juce::String trackLabel =
            track.getName().empty() ? "Track " + juce::String(i + 1) : juce::String(track.getName());
        g.drawText(trackLabel, 10, y + 4, getWidth() - 88, 20, juce::Justification::centredLeft);

        auto pluginBounds = getPluginLabelBounds(i);

        auto channelBounds = getChannelLabelBounds(i);
        g.setColour(juce::Colour(50, 50, 60));
        g.fillRoundedRectangle(channelBounds.toFloat(), 3.0f);
        g.setColour(juce::Colour(200, 200, 230));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText("Ch:" + juce::String(track.getChannel()), channelBounds, juce::Justification::centred);

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
        g.setColour(juce::Colour(50, 50, 60));
        g.fillRoundedRectangle(pluginBounds.toFloat(), 3.0f);
        g.setColour(labelActive ? juce::Colour(200, 200, 230) : juce::Colour(120, 120, 140));
        g.drawText(labelText, pluginBounds.reduced(4, 0), juce::Justification::centredLeft);

        auto muteBounds = getMuteButtonBounds(i);
        if (track.isMuted())
            g.setColour(juce::Colour(180, 60, 60));
        else
            g.setColour(juce::Colour(60, 60, 70));
        g.fillRoundedRectangle(muteBounds.toFloat(), 3.0f);
        g.setColour(track.isMuted() ? juce::Colours::white : juce::Colour(140, 140, 160));
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText("M", muteBounds, juce::Justification::centred);

        auto soloBounds = getSoloButtonBounds(i);
        if (track.isSolo())
            g.setColour(juce::Colour(200, 180, 60));
        else
            g.setColour(juce::Colour(60, 60, 70));
        g.fillRoundedRectangle(soloBounds.toFloat(), 3.0f);
        g.setColour(track.isSolo() ? juce::Colour(30, 30, 30) : juce::Colour(140, 140, 160));
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText("S", soloBounds, juce::Justification::centred);

        auto editorBounds = getEditorButtonBounds(i);
        g.setColour(juce::Colour(60, 60, 70));
        g.fillRoundedRectangle(editorBounds.toFloat(), 3.0f);
        g.setColour(pluginName.isEmpty() ? juce::Colour(140, 140, 160) : juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText("e", editorBounds, juce::Justification::centred);

        g.setColour(juce::Colour(60, 60, 70));
        g.drawHorizontalLine(y + trackRowHeight - 1, 0.0f, static_cast<float>(getWidth()));
    }

    auto addBounds = getAddButtonBounds();
    g.setColour(juce::Colour(34, 34, 42));
    g.fillRect(addBounds);
    g.setColour(juce::Colour(160, 160, 180));
    g.setFont(juce::Font(juce::FontOptions(13.0f)).boldened());
    g.drawText("+ Add Track", addBounds, juce::Justification::centred);
    g.setColour(juce::Colour(60, 60, 70));
    g.drawHorizontalLine(addBounds.getY(), 0.0f, static_cast<float>(getWidth()));
}

void TrackListComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence)
        return;

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
        if (selectedTrackIndices.count(row) > 0)
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
        if (selectedTrackIndices.count(row) > 0)
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

    if (e.mods.isCtrlDown())
    {
        if (selectedTrackIndices.count(row) > 0)
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

int TrackListComponent::getRowIndexAt(int y) const
{
    return y / trackRowHeight;
}

juce::Rectangle<int> TrackListComponent::getMuteButtonBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {getWidth() - 52, y + (trackRowHeight - 20) / 2, 24, 20};
}

juce::Rectangle<int> TrackListComponent::getSoloButtonBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {getWidth() - 26, y + (trackRowHeight - 20) / 2, 24, 20};
}

juce::Rectangle<int> TrackListComponent::getPluginLabelBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    auto channelBounds = getChannelLabelBounds(rowIndex);
    int width = juce::jmax(0, channelBounds.getX() - 14);
    return {10, y + 24, width, 16};
}

juce::Rectangle<int> TrackListComponent::getEditorButtonBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {getWidth() - 78, y + (trackRowHeight - 20) / 2, 24, 20};
}

juce::Rectangle<int> TrackListComponent::getChannelLabelBounds(int rowIndex) const
{
    int y = rowIndex * trackRowHeight;
    return {getWidth() - 114, y + 24, 32, 16};
}

juce::Rectangle<int> TrackListComponent::getAddButtonBounds() const
{
    int numTracks = sequence ? sequence->getNumTracks() : 0;
    int y = numTracks * trackRowHeight;
    return {0, y, getWidth(), addButtonRowHeight};
}
