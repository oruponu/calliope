#include "ui/tracklist/TrackListPanel.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/PassFocusUpTraverser.h"

TrackListPanel::TrackListPanel()
{
    trackListViewport.setViewedComponent(&trackList, false);
    trackListViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackListViewport);
    trackList.setWantsKeyboardFocus(true);
}

TrackListPanel::~TrackListPanel()
{
    if (sequence != nullptr)
        sequence->removeListener(this);
}

void TrackListPanel::setSequence(MidiSequence* seq)
{
    if (sequence != nullptr)
        sequence->removeListener(this);
    sequence = seq;
    if (sequence != nullptr)
        sequence->addListener(this);
    trackList.setSequence(sequence);
    repaint(headerBounds);
}

bool TrackListPanel::claimsFocus(const juce::Component* focused) const
{
    return &trackList == focused || trackList.isParentOf(focused);
}

TrackListComponent& TrackListPanel::getTrackList()
{
    return trackList;
}

void TrackListPanel::paint(juce::Graphics& g)
{
    using namespace calliope::theme;

    if (!headerBounds.isEmpty())
    {
        g.setColour(surface::bg2);
        g.fillRect(headerBounds);
        g.setColour(border::soft);
        g.drawHorizontalLine(headerBounds.getBottom() - 1, static_cast<float>(headerBounds.getX()),
                             static_cast<float>(headerBounds.getRight()));
        int numTracks = sequence != nullptr ? sequence->getNumTracks() : 0;
        g.setColour(text::t3);
        g.setFont(font::sans(font::sizeXS));
        g.drawText(juce::String::fromUTF8("TRACKS \xc2\xb7 ") + juce::String(numTracks), headerBounds.reduced(12, 0),
                   juce::Justification::centredLeft);
    }
}

void TrackListPanel::resized()
{
    auto area = getLocalBounds();
    headerBounds = area.removeFromTop(headerHeight);
    trackListViewport.setBounds(area);
    trackList.setSize(trackListViewport.getMaximumVisibleWidth(), trackList.getHeight());
    trackList.updateSize();
}

std::unique_ptr<juce::ComponentTraverser> TrackListPanel::createKeyboardFocusTraverser()
{
    return std::make_unique<PassFocusUpTraverser>();
}

void TrackListPanel::tracksChanged()
{
    repaint(headerBounds);
}
