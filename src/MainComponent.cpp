#include "MainComponent.h"
#include "io/MidiFileIO.h"

void MainComponent::TransportButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();
    float alpha = hover ? 1.0f : 0.65f;

    if (type == Stop)
    {
        g.setColour(juce::Colours::white.withAlpha(alpha * 0.7f));
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;
        g.fillRoundedRectangle(bounds.withSizeKeepingCentre(size, size), 2.0f);
    }
    else if (type == Play)
    {
        g.setColour(juce::Colours::white.withAlpha(alpha));
        auto h = bounds.getHeight() * 0.5f;
        auto w = h * 0.85f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        juce::Path path;
        path.addTriangle(cx - w * 0.38f, cy - h / 2, cx - w * 0.38f, cy + h / 2, cx + w * 0.62f, cy);
        g.fillPath(path);
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(alpha));
        auto h = bounds.getHeight() * 0.45f;
        auto barW = bounds.getWidth() * 0.14f;
        auto gap = bounds.getWidth() * 0.12f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        g.fillRoundedRectangle(cx - gap / 2 - barW, cy - h / 2, barW, h, 1.5f);
        g.fillRoundedRectangle(cx + gap / 2, cy - h / 2, barW, h, 1.5f);
    }
}

void MainComponent::TransportButton::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()) && onClick)
        onClick();
}

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
        updateTransportDisplay();
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
            playButton.setType(TransportButton::Play);
            vblankAttachment.reset();
            pianoRoll.setPlayheadTick(playbackEngine.getCurrentTick());
            updateTransportDisplay();
        }
        else
        {
            playbackEngine.play();
            playButton.setType(TransportButton::Pause);
            vblankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { onVBlank(); });
        }
    };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]()
    {
        playbackEngine.stop();
        playButton.setType(TransportButton::Play);
        vblankAttachment.reset();
        pianoRoll.setPlayheadTick(playbackEngine.getCurrentTick());
        updateTransportDisplay();
    };

    addAndMakeVisible(saveButton);
    saveButton.onClick = [this]() { saveFile(); };

    addAndMakeVisible(loadButton);
    loadButton.onClick = [this]() { loadFile(); };

    auto headerColour = juce::Colour(0xff8888aa);
    auto headerFont = juce::Font(juce::FontOptions(12.0f));

    for (auto* label : {&positionHeaderLabel, &timeSigHeaderLabel, &keyHeaderLabel, &tempoHeaderLabel})
    {
        addAndMakeVisible(label);
        label->setFont(headerFont);
        label->setColour(juce::Label::textColourId, headerColour);
        label->setJustificationType(juce::Justification::centred);
    }

    addAndMakeVisible(positionLabel);
    positionLabel.setFont(juce::Font(juce::FontOptions(28.0f)));
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    positionLabel.setJustificationType(juce::Justification::centred);

    for (auto* label : {&timeSigValueLabel, &keyValueLabel, &tempoValueLabel})
    {
        addAndMakeVisible(label);
        label->setFont(juce::Font(juce::FontOptions(24.0f)));
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setJustificationType(juce::Justification::centred);
    }

    bpmSlider.setRange(30.0, 300.0, 0.01);
    bpmSlider.setValue(sequence.getBpm());
    bpmSlider.onValueChange = [this]() { sequence.setBpm(bpmSlider.getValue()); };

    updateTransportDisplay();

    setSize(1280, 800);

    int c4Y = PianoRollComponent::headerHeight + (127 - 60) * PianoRollComponent::noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

MainComponent::~MainComponent()
{
    vblankAttachment.reset();
    playbackEngine.stop();
    playbackEngine.removeListener(&midiOutput);
    midiOutput.close();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colour(0xff1c1c2c));
    g.fillRect(0, 0, getWidth(), transportBarHeight);

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
    auto toolbar = area.removeFromTop(transportBarHeight);

    auto rightEdge = toolbar.removeFromRight(150);
    saveButton.setBounds(rightEdge.removeFromLeft(70).reduced(4, 18));
    loadButton.setBounds(rightEdge.removeFromLeft(70).reduced(4, 18));

    const int posW = 190;
    const int btnW = 84;
    const int tsW = 55;
    const int keyW = 55;
    const int tempoW = 90;
    const int g1 = 28, g2 = 36, g3 = 36, g4 = 36;
    const int contentWidth = posW + g1 + btnW + g2 + tsW + g3 + keyW + g4 + tempoW;

    auto content = toolbar.withSizeKeepingCentre(contentWidth, transportBarHeight);

    const int topPad = 8;
    const int headerH = 18;

    auto layoutSection = [&](int width, juce::Label& header, juce::Label& value)
    {
        auto section = content.removeFromLeft(width);
        section.removeFromTop(topPad);
        header.setBounds(section.removeFromTop(headerH));
        value.setBounds(section);
    };

    layoutSection(posW, positionHeaderLabel, positionLabel);
    content.removeFromLeft(g1);

    auto btnSection = content.removeFromLeft(btnW);
    auto btnArea = btnSection.withSizeKeepingCentre(84, 40);
    stopButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    playButton.setBounds(btnArea.removeFromLeft(40));
    content.removeFromLeft(g2);

    layoutSection(tsW, timeSigHeaderLabel, timeSigValueLabel);
    content.removeFromLeft(g3);

    layoutSection(keyW, keyHeaderLabel, keyValueLabel);
    content.removeFromLeft(g4);

    layoutSection(tempoW, tempoHeaderLabel, tempoValueLabel);

    viewport.setBounds(area);
}

void MainComponent::onVBlank()
{
    double tick = playbackEngine.getCurrentTick();
    pianoRoll.setPlayheadTick(tick);
    updateTransportDisplay();
    bpmSlider.setValue(sequence.getTempoAt(static_cast<int>(tick)), juce::dontSendNotification);

    int playheadX = pianoRoll.tickToX(static_cast<int>(tick));
    int viewX = viewport.getViewPositionX();
    int viewRight = viewX + viewport.getViewWidth();

    if (playheadX < viewX + PianoRollComponent::keyboardWidth || playheadX > viewRight - 20)
        viewport.setViewPosition(playheadX - PianoRollComponent::keyboardWidth, viewport.getViewPositionY());
}

void MainComponent::updateTransportDisplay()
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());

    auto bbt = sequence.tickToBarBeatTick(tick);
    positionLabel.setText(juce::String(bbt.bar).paddedLeft('0', 3) + "." + juce::String(bbt.beat).paddedLeft('0', 2) +
                              "." + juce::String(bbt.tick).paddedLeft('0', 4),
                          juce::dontSendNotification);

    auto ts = sequence.getTimeSignatureAt(tick);
    timeSigValueLabel.setText(juce::String(ts.numerator) + "/" + juce::String(ts.denominator),
                              juce::dontSendNotification);

    keyValueLabel.setText("--", juce::dontSendNotification);

    double tempo = sequence.getTempoAt(tick);
    tempoValueLabel.setText(juce::String(tempo, 2), juce::dontSendNotification);
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
    playButton.setType(TransportButton::Play);
    vblankAttachment.reset();

    bpmSlider.setValue(sequence.getBpm(), juce::dontSendNotification);
    pianoRoll.setSequence(&sequence);
    pianoRoll.setPlayheadTick(0);
    updateTransportDisplay();

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
