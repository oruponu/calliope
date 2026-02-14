#pragma once

#include "../model/MidiSequence.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <set>

class TrackListComponent : public juce::Component
{
public:
    void setSequence(MidiSequence* seq);
    void refresh();

    int getActiveTrackIndex() const;
    void setActiveTrackIndex(int index);
    const std::set<int>& getSelectedTrackIndices() const;
    bool isTrackSelected(int index) const;
    void setSelectedTrackIndices(const std::set<int>& indices);

    std::function<void(int activeIndex, const std::set<int>& selectedIndices)> onTrackSelected;
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
    void notifySelectionChanged();

    MidiSequence* sequence = nullptr;
    std::set<int> selectedTrackIndices = {0};
    int activeTrackIndex = 0;
    int anchorTrackIndex = 0;
};
