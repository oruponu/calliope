#include "app/PlayheadLinks.h"
#include "engine/PlaybackEngine.h"
#include "ui/eventlist/EventListComponent.h"
#include "ui/pianoroll/PianoRollPanel.h"
#include "ui/transport/TransportBarComponent.h"

PlayheadLinks::PlayheadLinks(PianoRollPanel& pianoRollPanelRef, EventListComponent& eventListRef,
                             TransportBarComponent& transportBarRef, PlaybackEngine& playbackEngineRef,
                             juce::Component& vblankOwnerRef)
    : pianoRollPanel(pianoRollPanelRef), eventList(eventListRef), transportBar(transportBarRef),
      playbackEngine(playbackEngineRef), vblankOwner(vblankOwnerRef)
{
    pianoRollPanel.getPianoRoll().onPlayheadMoved = [this](int tick)
    {
        transportBar.stopCountIn();
        playbackEngine.setPositionInTicks(tick);
        pianoRollPanel.getControllerLane().setPlayheadTick(tick);
        eventList.setPlayheadTick(tick);
        transportBar.updateDisplay();
    };
    eventList.onEventSelected = [this](int tick)
    {
        transportBar.stopCountIn();
        playbackEngine.setPositionInTicks(tick);
        pianoRollPanel.getPianoRoll().setPlayheadTick(tick);
        transportBar.updateDisplay();
        pianoRollPanel.scrollToPlayhead(tick);
    };
    transportBar.onPlayheadMoved = [this](double tick)
    {
        pianoRollPanel.getPianoRoll().setPlayheadTick(tick);
        pianoRollPanel.getControllerLane().setPlayheadTick(tick);
        eventList.setPlayheadTick(tick);
    };
    transportBar.onScrollToPlayhead = [this](int tick) { pianoRollPanel.scrollToPlayhead(tick); };
    transportBar.onReturnToStart = [this]() { pianoRollPanel.scrollToStart(); };
    transportBar.onPlaybackStateChanged = [this](bool playing)
    {
        if (playing)
            vblankAttachment = std::make_unique<juce::VBlankAttachment>(&vblankOwner, [this]() { onVBlank(); });
        else
            vblankAttachment.reset();
    };
    transportBar.onLoopRegionChanged = [this](bool enabled, int startTick, int endTick)
    {
        pianoRollPanel.getPianoRoll().setLoopRegion(enabled, startTick, endTick);
        pianoRollPanel.getControllerLane().setLoopRegion(enabled, startTick, endTick);
    };
    pianoRollPanel.getPianoRoll().onLoopRegionChanged = [this](int startTick, int endTick)
    {
        playbackEngine.setLoopRange(startTick, endTick);
        bool enabled = playbackEngine.isLoopEnabled();
        pianoRollPanel.getControllerLane().setLoopRegion(enabled, startTick, endTick);
    };
}

void PlayheadLinks::stopFollowing()
{
    vblankAttachment.reset();
}

void PlayheadLinks::onVBlank()
{
    double tick = playbackEngine.getCurrentTick();
    pianoRollPanel.getPianoRoll().setPlayheadTick(tick);
    pianoRollPanel.getControllerLane().setPlayheadTick(tick);
    eventList.setPlayheadTick(tick);
    transportBar.updateDisplay();
    pianoRollPanel.scrollToPlayhead(static_cast<int>(tick));
}
