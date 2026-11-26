#pragma once

#include "model/grid/GridDivision.h"
#include "ui/pianoroll/ControllerLaneComponent.h"
#include "ui/pianoroll/ControllerLaneViewport.h"
#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/PianoRollComponent.h"
#include "ui/pianoroll/PianoRollToolBar.h"
#include "ui/pianoroll/PianoRollViewport.h"
#include "ui/widgets/Divider.h"
#include "ui/widgets/ZoomStrip.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <set>

class PianoRollPanel : public juce::Component
{
public:
    explicit PianoRollPanel(UndoHistory& history);

    void setSequence(MidiSequence* sequence);
    void setSelectedTracks(int activeIndex, const std::set<int>& selected);
    void setPlayheadTick(double tick);
    void setLoopRegion(bool enabled, int startTick, int endTick);
    // Scrolls so that middle C sits halfHeight below the top of the view, as at startup.
    void scrollToMiddleC(int halfHeight);
    void scrollToPlayhead(int tick);
    void scrollToStart();
    GridDivision getGridDivision() const;

    void setActiveTool(PianoRollComponent::EditMode mode);
    void toggleSnap();

    // Edit commands. The caller checks that the panel has the focus where the command needs it.
    bool canCutOrCopy() const;
    bool canPaste() const;
    bool canSelectAll() const;
    bool hasSelectedNotes() const;
    bool hasNotesInActiveTrack() const;
    void cut();
    void copy();
    void paste(int tick);
    void selectAll();
    void nudgeSelectedNotesPitch(int semitones);
    void moveSelectionToAdjacentNote(int direction);
    void quantizeSelectedNotes();
    void deselectNotes();
    void clearSelections(); // Clears every selection in the piano roll and the controller lane, not just the notes.
    void scrollViewUp();
    void scrollViewDown();
    void scrollViewLeft();
    void scrollViewRight();
    void zoomInHorizontally();
    void zoomOutHorizontally();
    void zoomInVertically();
    void zoomOutVertically();
    void resetZoom();

    // Whether focused is the piano roll, the controller lane or a part of the tool bar, or inside one. When it is,
    // remembers whether it is in the controller lane, for Select All.
    bool claimsFocus(const juce::Component* focused);

    // For the links between the panels and for the recording layer only. Edits go through the methods above.
    PianoRollComponent& getPianoRoll();
    ControllerLaneComponent& getControllerLane();

    std::function<void(GridDivision)> onGridDivisionChanged;

    void resized() override;
    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

private:
    void scrollNoteIntoView(int startTick, int noteNumber);
    void scrollViewVertically(int deltaY);
    void scrollViewHorizontally(int deltaX);
    void setHorizontalZoom(int newBeatWidth, int anchorXInViewport);
    void setVerticalZoom(int newNoteHeight, int anchorYInViewport);
    void zoomHorizontal(float factor, int anchorXInViewport);
    void zoomVertical(float factor, int anchorYInViewport);
    bool selectAllTakesControllerPoints() const;

    PianoRollToolBar toolBar;
    // Shared by the piano roll and the controller lane, so one copy replaces the other's.
    EditClipboard clipboard;
    PianoRollComponent pianoRoll;
    PianoRollViewport viewport;
    ControllerLaneComponent controllerLane;
    ControllerLaneViewport controllerLaneViewport;
    Divider controllerLaneDivider{Divider::Horizontal};
    ZoomStrip horizontalZoomStrip{ZoomStrip::Horizontal};
    ZoomStrip verticalZoomStrip{ZoomStrip::Vertical};
    int controllerLaneHeight = 120;
    int controllerLaneHeightOnDragStart = 120;
    bool syncingScroll = false;
    // Whether the controller lane, rather than the piano roll or the tool bar, took the focus last.
    bool controllerLaneFocused = false;

    static constexpr int dividerThickness = 5;
    static constexpr int zoomStripLength = 100;

    JUCE_DECLARE_NON_COPYABLE(PianoRollPanel)
};
