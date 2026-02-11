#include "MainComponent.h"

MainComponent::MainComponent()
{
    midiOutput.open();

    buildTestSequence();

    playbackEngine.setSequence(&sequence);
    playbackEngine.addListener(&midiOutput);

    pianoRoll.setSequence(&sequence);
    viewport.setViewedComponent(&pianoRoll, false);
    viewport.setScrollBarsShown(true, true);
    addAndMakeVisible(viewport);

    addAndMakeVisible(playButton);
    playButton.onClick = [this]()
    {
        if (playbackEngine.isPlaying())
        {
            playbackEngine.stop();
            playButton.setButtonText("Play");
        }
        else
        {
            playbackEngine.setPositionInTicks(0);
            playbackEngine.play();
            playButton.setButtonText("Stop");
        }
    };

    setSize(800, 600);

    int c4Y = (127 - 60) * PianoRollComponent::noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

MainComponent::~MainComponent()
{
    playbackEngine.stop();
    playbackEngine.removeListener(&midiOutput);
    midiOutput.close();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    auto toolbar = area.removeFromTop(40);
    playButton.setBounds(toolbar.removeFromLeft(90).reduced(5));
    viewport.setBounds(area);
}

void MainComponent::buildTestSequence()
{
    auto& track = sequence.addTrack();

    // C major scale
    track.addNote({60, 100, 0, 480});    // C4
    track.addNote({62, 100, 480, 480});  // D4
    track.addNote({64, 100, 960, 480});  // E4
    track.addNote({65, 100, 1440, 480}); // F4
    track.addNote({67, 100, 1920, 480}); // G4
    track.addNote({69, 100, 2400, 480}); // A4
    track.addNote({71, 100, 2880, 480}); // B4
    track.addNote({72, 100, 3360, 480}); // C5
}
