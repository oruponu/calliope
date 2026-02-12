#include "TrackListComponent.h"
#include "TrackColours.h"

void TrackListComponent::setSequence(MidiSequence* seq)
{
    sequence = seq;
    selectedTrackIndex = (seq && seq->getNumTracks() > 0) ? 0 : -1;
    repaint();
}

void TrackListComponent::refresh()
{
    repaint();
}

int TrackListComponent::getSelectedTrackIndex() const
{
    return selectedTrackIndex;
}

void TrackListComponent::setSelectedTrackIndex(int index)
{
    if (!sequence || index < 0 || index >= sequence->getNumTracks())
        return;
    selectedTrackIndex = index;
    repaint();
    if (onTrackSelected)
        onTrackSelected(selectedTrackIndex);
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

        if (i == selectedTrackIndex)
            g.setColour(juce::Colour(55, 55, 70));
        else
            g.setColour(juce::Colour(40, 40, 48));
        g.fillRect(rowBounds);

        g.setColour(TrackColours::getColour(i));
        g.fillRect(0, y, 4, trackRowHeight);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(13.0f)).boldened());
        juce::String trackLabel =
            track.getName().empty() ? "Track " + juce::String(i + 1) : juce::String(track.getName());
        g.drawText(trackLabel, 10, y + 4, getWidth() - 62, 20, juce::Justification::centredLeft);

        int channel = -1;
        if (track.getNumNotes() > 0)
            channel = track.getNote(0).channel;

        juce::String info = "Ch:" + (channel > 0 ? juce::String(channel) : juce::String("--")) +
                            "  Notes:" + juce::String(track.getNumNotes());
        g.setColour(juce::Colour(140, 140, 160));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(info, 10, y + 24, getWidth() - 62, 16, juce::Justification::centredLeft);

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

        g.setColour(juce::Colour(60, 60, 70));
        g.drawHorizontalLine(y + trackRowHeight - 1, 0.0f, static_cast<float>(getWidth()));
    }
}

void TrackListComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence)
        return;

    int row = getRowIndexAt(e.y);
    if (row < 0 || row >= sequence->getNumTracks())
        return;

    auto& track = sequence->getTrack(row);

    if (getMuteButtonBounds(row).contains(e.x, e.y))
    {
        track.setMuted(!track.isMuted());
        repaint();
        if (onMuteSoloChanged)
            onMuteSoloChanged();
        return;
    }

    if (getSoloButtonBounds(row).contains(e.x, e.y))
    {
        track.setSolo(!track.isSolo());
        repaint();
        if (onMuteSoloChanged)
            onMuteSoloChanged();
        return;
    }

    if (row != selectedTrackIndex)
    {
        selectedTrackIndex = row;
        repaint();
        if (onTrackSelected)
            onTrackSelected(selectedTrackIndex);
    }
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
