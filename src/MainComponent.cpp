#include "MainComponent.h"
#include "io/MidiFileIO.h"

MainComponent::MainComponent()
{
    midiOutput.open();

    buildTestSequence();

    playbackEngine.setSequence(&sequence);
    playbackEngine.addListener(&midiOutput);

    pianoRoll.setSequence(&sequence);
    pianoRoll.onPlayheadMoved = [this](int tick)
    {
        playbackEngine.setPositionInTicks(tick);
        updatePositionLabel();
    };
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
            stopTimer();
            pianoRoll.setPlayheadTick(playbackEngine.getCurrentTick());
            updatePositionLabel();
        }
        else
        {
            playbackEngine.play();
            playButton.setButtonText("Pause");
            startTimerHz(30);
        }
    };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]()
    {
        playbackEngine.stop();
        playButton.setButtonText("Play");
        stopTimer();
        pianoRoll.setPlayheadTick(playbackEngine.getCurrentTick());
        updatePositionLabel();
    };

    addAndMakeVisible(saveButton);
    saveButton.onClick = [this]() { saveFile(); };

    addAndMakeVisible(loadButton);
    loadButton.onClick = [this]() { loadFile(); };

    addAndMakeVisible(bpmLabel);
    bpmLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(bpmSlider);
    bpmSlider.setRange(30.0, 300.0, 1.0);
    bpmSlider.setValue(sequence.getBpm());
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 45, 24);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.onValueChange = [this]() { sequence.setBpm(bpmSlider.getValue()); };

    addAndMakeVisible(positionLabel);
    positionLabel.setJustificationType(juce::Justification::centredLeft);
    updatePositionLabel();

    setSize(1280, 800);

    int c4Y = PianoRollComponent::headerHeight + (127 - 60) * PianoRollComponent::noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

MainComponent::~MainComponent()
{
    stopTimer();
    playbackEngine.stop();
    playbackEngine.removeListener(&midiOutput);
    midiOutput.close();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    if (fileDragOver)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colours::dodgerblue);
        g.drawRect(getLocalBounds(), 2);
    }
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& file : files)
        if (file.endsWithIgnoreCase(".mid") || file.endsWithIgnoreCase(".midi"))
            return true;
    return false;
}

void MainComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    fileDragOver = true;
    repaint();
}

void MainComponent::fileDragExit(const juce::StringArray&)
{
    fileDragOver = false;
    repaint();
}

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    fileDragOver = false;
    repaint();

    for (auto& path : files)
    {
        if (path.endsWithIgnoreCase(".mid") || path.endsWithIgnoreCase(".midi"))
        {
            juce::File file(path);
            if (MidiFileIO::load(sequence, file))
                onSequenceLoaded();
            break;
        }
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    auto toolbar = area.removeFromTop(40);

    playButton.setBounds(toolbar.removeFromLeft(70).reduced(5));
    stopButton.setBounds(toolbar.removeFromLeft(70).reduced(5));

    toolbar.removeFromLeft(10);
    saveButton.setBounds(toolbar.removeFromLeft(70).reduced(5));
    loadButton.setBounds(toolbar.removeFromLeft(70).reduced(5));

    toolbar.removeFromLeft(10);
    bpmLabel.setBounds(toolbar.removeFromLeft(35));
    bpmSlider.setBounds(toolbar.removeFromLeft(160).reduced(5));

    toolbar.removeFromLeft(10);
    positionLabel.setBounds(toolbar.removeFromLeft(150));

    viewport.setBounds(area);
}

void MainComponent::timerCallback()
{
    int tick = playbackEngine.getCurrentTick();
    pianoRoll.setPlayheadTick(tick);
    updatePositionLabel();
    bpmSlider.setValue(sequence.getTempoAt(tick), juce::dontSendNotification);
}

void MainComponent::updatePositionLabel()
{
    int tick = playbackEngine.getCurrentTick();
    auto bbt = sequence.tickToBarBeatTick(tick);
    positionLabel.setText(juce::String(bbt.bar).paddedLeft('0', 3) + "." + juce::String(bbt.beat).paddedLeft('0', 2) +
                              "." + juce::String(bbt.tick).paddedLeft('0', 4),
                          juce::dontSendNotification);
}

void MainComponent::saveFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save MIDI File", juce::File{}, "*.mid");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{})
                                     MidiFileIO::save(sequence, file);
                             });
}

void MainComponent::loadFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Open MIDI File", juce::File{}, "*.mid");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{} && MidiFileIO::load(sequence, file))
                                     onSequenceLoaded();
                             });
}

void MainComponent::onSequenceLoaded()
{
    playbackEngine.stop();
    playbackEngine.setPositionInTicks(0);
    playButton.setButtonText("Play");
    stopTimer();

    bpmSlider.setValue(sequence.getBpm(), juce::dontSendNotification);
    pianoRoll.setSequence(&sequence);
    pianoRoll.setPlayheadTick(0);
    updatePositionLabel();

    int c4Y = PianoRollComponent::headerHeight + (127 - 60) * PianoRollComponent::noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
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
