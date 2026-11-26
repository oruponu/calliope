#pragma once

#include "model/MidiSequence.h"
#include "ui/tracklist/TrackListComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

class TrackListPanel : public juce::Component, private MidiSequence::Listener
{
public:
    TrackListPanel();
    ~TrackListPanel() override;

    // Sets the track list's sequence and listens to it for the number of tracks shown in the header.
    void setSequence(MidiSequence* sequence);
    // Whether focused is the track list or inside it.
    bool claimsFocus(const juce::Component* focused) const;
    TrackListComponent& getTrackList();

    void paint(juce::Graphics& g) override;
    void resized() override;
    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

private:
    void tracksChanged() override;

    TrackListComponent trackList;
    juce::Viewport trackListViewport;
    juce::Rectangle<int> headerBounds;
    MidiSequence* sequence = nullptr;

    static constexpr int headerHeight = 32;

    JUCE_DECLARE_NON_COPYABLE(TrackListPanel)
};
