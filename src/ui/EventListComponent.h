#pragma once

#include "../model/MidiSequence.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <set>
#include <vector>

struct EventListItem
{
    enum Kind
    {
        Note,
        ControlChange,
        ProgramChange,
        PitchBend,
        ChannelPressure,
        KeyPressure
    };

    Kind kind = Note;
    int tick = 0;
    int trackIndex = 0;
    int sourceIndex = 0;
    int noteNumber = 0;
    int duration = 0;
    int velocity = 0;
    int channel = 1;
    int data1 = 0;
    int data2 = 0;
};

class EventListComponent : public juce::Component, private juce::ListBoxModel
{
public:
    EventListComponent();

    void setSequence(MidiSequence* seq);
    void setSelectedTracks(const std::set<int>& selected);
    void refresh();
    void setPlayheadTick(double tick);
    void setSelectedNotes(const std::set<std::pair<int, int>>& selected);

    std::function<void(int tick)> onEventSelected;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int preferredWidth = 280;
    static constexpr int rowHeight = 20;
    static constexpr int headerHeight = 22;

private:
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void rebuildList();

    static juce::String formatPosition(const BarBeatTick& bbt);
    static juce::String formatEvent(const EventListItem& item);
    static juce::String formatLength(const EventListItem& item);
    static juce::String formatValue(const EventListItem& item);
    static juce::String noteNumberToName(int noteNumber);

    MidiSequence* sequence = nullptr;
    std::set<int> selectedTracks;
    std::vector<EventListItem> items;

    juce::ListBox listBox;
    int lastPlayheadRow = -1;
    bool updatingFromPlayhead = false;
    bool updatingFromNoteSelection = false;

    static constexpr int colPosition = 90;
    static constexpr int colEvent = 80;
    static constexpr int colLength = 50;
    static constexpr int colValue = 46;
};
