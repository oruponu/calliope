#pragma once

#include "../model/MidiSequence.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class TrackListComponent : public juce::Component
{
public:
    void setSequence(MidiSequence* seq);
    void refresh();

    int getSelectedTrackIndex() const;
    void setSelectedTrackIndex(int index);

    std::function<void(int trackIndex)> onTrackSelected;
    std::function<void()> onMuteSoloChanged;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    static constexpr int trackRowHeight = 48;
    static constexpr int preferredWidth = 180;

private:
    int getRowIndexAt(int y) const;
    juce::Rectangle<int> getMuteButtonBounds(int rowIndex) const;
    juce::Rectangle<int> getSoloButtonBounds(int rowIndex) const;
    void updateSize();

    MidiSequence* sequence = nullptr;
    int selectedTrackIndex = 0;
};
