#include "TransportBarComponent.h"

#include "../document/Document.h"
#include "../engine/PlaybackEngine.h"
#include "../model/UndoActions.h"
#include "Theme.h"

TransportBarComponent::TransportBarComponent(Document& documentRef, PlaybackEngine& playbackEngineRef)
    : document(documentRef), playbackEngine(playbackEngineRef)
{
    using namespace calliope::theme;

    addAndMakeVisible(returnToStartButton);
    returnToStartButton.onClick = [this]() { returnToStart(); };

    addAndMakeVisible(playButton);
    playButton.onClick = [this]() { togglePlay(); };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]() { stopAndNotify(); };

    addAndMakeVisible(loopButton);
    loopButton.onClick = [this]() { toggleLoop(); };

    auto headerColour = text::t2;
    auto headerFont = font::sans(font::sizeXS);

    for (auto* label : {&positionHeaderLabel, &timeSigHeaderLabel, &keyHeaderLabel, &tempoHeaderLabel})
    {
        addAndMakeVisible(label);
        label->setFont(headerFont);
        label->setColour(juce::Label::textColourId, headerColour);
        label->setJustificationType(juce::Justification::centred);
    }

    for (auto* sep : {&positionDot1, &positionDot2, &timeSigSlashLabel})
    {
        addAndMakeVisible(sep);
        sep->setFont(font::mono(font::sizeDisplay).boldened());
        sep->setColour(juce::Label::textColourId, text::t1);
        sep->setJustificationType(juce::Justification::centred);
        sep->setBorderSize(juce::BorderSize<int>(0));
        sep->setMinimumHorizontalScale(1.0f);
    }

    struct PositionField
    {
        WheelLabel& label;
        int maxLength;
        PositionUnit unit;
    };
    for (auto field : {PositionField{positionBarLabel, 3, PositionUnit::Bar},
                       PositionField{positionBeatLabel, 2, PositionUnit::Beat},
                       PositionField{positionTickLabel, 4, PositionUnit::Tick}})
    {
        auto& label = field.label;
        addAndMakeVisible(label);
        label.setFont(font::mono(font::sizeDisplay).boldened());
        label.setColour(juce::Label::textColourId, text::t1);
        label.setJustificationType(juce::Justification::centred);
        label.setBorderSize(juce::BorderSize<int>(0));
        label.setMinimumHorizontalScale(1.0f);
        label.setEditable(true);
        int maxLength = field.maxLength;
        label.onEditorShow = [&label, maxLength]()
        {
            if (auto* editor = label.getCurrentTextEditor())
            {
                editor->setInputRestrictions(maxLength, "0123456789");
                editor->setJustification(juce::Justification::centred);
                editor->selectAll();
            }
        };
        label.onTextChange = [this]() { commitPositionEdit(); };
        PositionUnit unit = field.unit;
        label.onWheel = [this, unit](int direction) { nudgePosition(unit, direction); };
    }

    addAndMakeVisible(tempoValueLabel);
    tempoValueLabel.setFont(font::mono(font::sizeDisplay).boldened());
    tempoValueLabel.setColour(juce::Label::textColourId, text::t1);
    tempoValueLabel.setJustificationType(juce::Justification::centred);
    tempoValueLabel.setBorderSize(juce::BorderSize<int>(0));
    tempoValueLabel.setMinimumHorizontalScale(1.0f);
    tempoValueLabel.setEditable(true);
    tempoValueLabel.onEditorShow = [this]()
    {
        if (auto* editor = tempoValueLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(7, "0123456789.");
            editor->setJustification(juce::Justification::centred);
            editor->selectAll();
        }
    };
    tempoValueLabel.onTextChange = [this]() { commitTempoEdit(); };
    tempoValueLabel.onWheel = [this](int direction) { nudgeTempo(direction); };

    addAndMakeVisible(keyValueLabel);
    keyValueLabel.setFont(font::mono(font::sizeDisplay).boldened());
    keyValueLabel.setColour(juce::Label::textColourId, text::t1);
    keyValueLabel.setJustificationType(juce::Justification::centred);
    keyValueLabel.setBorderSize(juce::BorderSize<int>(0));
    keyValueLabel.setMinimumHorizontalScale(1.0f);
    keyValueLabel.setEditable(true);
    keyValueLabel.onEditorShow = [this]()
    {
        if (auto* editor = keyValueLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(3, "ABCDEFGabcdefg#m");
            editor->setJustification(juce::Justification::centred);
            editor->selectAll();
        }
    };
    keyValueLabel.onTextChange = [this]() { commitKeySignatureEdit(); };
    keyValueLabel.onWheel = [this](int direction) { nudgeKeySignature(direction); };

    struct TimeSigField
    {
        WheelLabel* label;
        int maxLength;
        juce::Justification justification;
        TimeSigUnit unit;
    };
    for (auto field : {TimeSigField{&timeSigNumLabel, 2, juce::Justification::centredRight, TimeSigUnit::Numerator},
                       TimeSigField{&timeSigDenLabel, 2, juce::Justification::centredLeft, TimeSigUnit::Denominator}})
    {
        auto* label = field.label;
        int maxLength = field.maxLength;
        auto justification = field.justification;
        addAndMakeVisible(label);
        label->setFont(font::mono(font::sizeDisplay).boldened());
        label->setColour(juce::Label::textColourId, text::t1);
        label->setJustificationType(justification);
        label->setBorderSize(juce::BorderSize<int>(0));
        label->setMinimumHorizontalScale(1.0f);
        label->setEditable(true);
        label->onEditorShow = [label, maxLength, justification]()
        {
            if (auto* editor = label->getCurrentTextEditor())
            {
                editor->setInputRestrictions(maxLength, "0123456789");
                editor->setJustification(justification);
                editor->selectAll();
            }
        };
        label->onTextChange = [this]() { commitTimeSignatureEdit(); };
        TimeSigUnit unit = field.unit;
        label->onWheel = [this, unit](int direction) { nudgeTimeSignature(unit, direction); };
    }

    document.getSequence().addListener(this);
}

TransportBarComponent::~TransportBarComponent()
{
    document.getSequence().removeListener(this);
}

void TransportBarComponent::togglePlay()
{
    if (playbackEngine.isPlaying())
    {
        stopAndNotify();
    }
    else
    {
        playbackEngine.play();
        playButton.setActive(true);
        if (onPlaybackStateChanged)
            onPlaybackStateChanged(true);
    }
}

void TransportBarComponent::stopAndNotify()
{
    playbackEngine.stop();
    playButton.setActive(false);
    if (onPlaybackStateChanged)
        onPlaybackStateChanged(false);
    if (onPlayheadMoved)
        onPlayheadMoved(playbackEngine.getCurrentTick());
    updateDisplay();
}

void TransportBarComponent::toggleLoop()
{
    bool newState = !playbackEngine.isLoopEnabled();
    playbackEngine.setLoopEnabled(newState);
    loopButton.setActive(newState);
    if (onLoopRegionChanged)
        onLoopRegionChanged(newState, playbackEngine.getLoopStartTick(), playbackEngine.getLoopEndTick());
}

void TransportBarComponent::returnToStart()
{
    playbackEngine.setPositionInTicks(0);
    if (onPlayheadMoved)
        onPlayheadMoved(0);
    updateDisplay();
    if (onReturnToStart)
        onReturnToStart();
}

void TransportBarComponent::jumpToTick(int tick)
{
    playbackEngine.setPositionInTicks(tick);
    if (onPlayheadMoved)
        onPlayheadMoved(tick);
    updateDisplay();
    if (onScrollToPlayhead)
        onScrollToPlayhead(tick);
}

void TransportBarComponent::setPlaying(bool playing)
{
    playButton.setActive(playing);
}

void TransportBarComponent::setLoopActive(bool active)
{
    loopButton.setActive(active);
}

void TransportBarComponent::commitPositionEdit()
{
    int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
    auto current = document.getSequence().tickToBarBeatTick(currentTick);

    int bar = positionBarLabel.getText().isEmpty() ? current.bar : positionBarLabel.getText().getIntValue();
    int beat = positionBeatLabel.getText().isEmpty() ? current.beat : positionBeatLabel.getText().getIntValue();
    int tickInBeat = positionTickLabel.getText().isEmpty() ? current.tick : positionTickLabel.getText().getIntValue();

    jumpToTick(document.getSequence().barBeatTickToTick(bar, beat, tickInBeat));
}

void TransportBarComponent::nudgePosition(PositionUnit unit, int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = document.getSequence().getTimeSignatureAt(tick);
    int ticksPerBeat = document.getSequence().getTicksPerQuarterNote() * 4 / ts.denominator;

    int step = 1;
    switch (unit)
    {
    case PositionUnit::Bar:
        step = ticksPerBeat * ts.numerator;
        break;
    case PositionUnit::Beat:
        step = ticksPerBeat;
        break;
    case PositionUnit::Tick:
        step = 1;
        break;
    }

    jumpToTick(juce::jmax(0, tick + direction * step));
}

void TransportBarComponent::commitTempoEdit()
{
    double bpm = tempoValueLabel.getText().getDoubleValue();
    if (bpm > 0.0)
        setTempoAtPlayhead(bpm);
    else
        updateDisplay();
}

void TransportBarComponent::nudgeTempo(int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto tc = document.getSequence().getTempoChangeAt(tick);

    setTempoAtPlayhead(juce::roundToInt(tc.bpm) + direction);
}

void TransportBarComponent::setTempoAtPlayhead(double bpm)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto tc = document.getSequence().getTempoChangeAt(tick);

    double clamped = juce::jlimit(MidiSequence::minBpm, MidiSequence::maxBpm, bpm);
    if (clamped == tc.bpm)
    {
        updateDisplay();
        return;
    }

    document.getUndoManager().beginNewTransaction();
    document.getUndoManager().perform(new TempoChangeAction(&document.getSequence(), tc.tick, clamped));
    playbackEngine.rebuildSnapshot();
}

void TransportBarComponent::commitTimeSignatureEdit()
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = document.getSequence().getTimeSignatureAt(tick);

    int num = timeSigNumLabel.getText().isEmpty() ? ts.numerator : timeSigNumLabel.getText().getIntValue();
    int den = timeSigDenLabel.getText().isEmpty() ? ts.denominator : timeSigDenLabel.getText().getIntValue();

    setTimeSignatureAtPlayhead(num, den);
}

void TransportBarComponent::nudgeTimeSignature(TimeSigUnit unit, int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = document.getSequence().getTimeSignatureAt(tick);

    if (unit == TimeSigUnit::Numerator)
        setTimeSignatureAtPlayhead(ts.numerator + direction, ts.denominator);
    else
        setTimeSignatureAtPlayhead(ts.numerator, direction > 0 ? ts.denominator * 2 : ts.denominator / 2);
}

void TransportBarComponent::setTimeSignatureAtPlayhead(int num, int den)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = document.getSequence().getTimeSignatureAt(tick);

    auto snapToPowerOfTwo = [](int value)
    {
        value = juce::jlimit(2, 64, value);
        int lower = 1;
        while (lower * 2 <= value)
            lower *= 2;
        int upper = juce::jmin(64, lower * 2);
        return (value - lower <= upper - value) ? lower : upper;
    };

    num = juce::jlimit(1, 64, num);
    den = snapToPowerOfTwo(den);

    if (num == ts.numerator && den == ts.denominator)
    {
        updateDisplay();
        return;
    }

    document.getUndoManager().beginNewTransaction();
    document.getUndoManager().perform(new TimeSignatureChangeAction(&document.getSequence(), ts.tick, num, den));
}

void TransportBarComponent::commitKeySignatureEdit()
{
    int sharpsOrFlats = 0;
    bool isMinor = false;
    if (MidiSequence::keySignatureFromString(keyValueLabel.getText().toStdString(), sharpsOrFlats, isMinor))
        setKeySignatureAtPlayhead(sharpsOrFlats, isMinor);
    else
        updateDisplay();
}

void TransportBarComponent::nudgeKeySignature(int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ks = document.getSequence().getKeySignatureAt(tick);

    int sf = juce::jlimit(-6, 6, ks.sharpsOrFlats);
    int index = juce::jlimit(0, 25, (sf + 6) + (ks.isMinor ? 13 : 0) + direction);

    bool isMinor = index >= 13;
    setKeySignatureAtPlayhead((isMinor ? index - 13 : index) - 6, isMinor);
}

void TransportBarComponent::setKeySignatureAtPlayhead(int sharpsOrFlats, bool isMinor)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ks = document.getSequence().getKeySignatureAt(tick);

    bool hasActiveKey = false;
    for (const auto& k : document.getSequence().getKeySignatureChanges())
        if (k.tick <= tick)
        {
            hasActiveKey = true;
            break;
        }

    sharpsOrFlats = MidiSequence::normalizeSharpsOrFlats(sharpsOrFlats);
    if (hasActiveKey && sharpsOrFlats == ks.sharpsOrFlats && isMinor == ks.isMinor)
    {
        updateDisplay();
        return;
    }

    document.getUndoManager().beginNewTransaction();
    document.getUndoManager().perform(
        new KeySignatureChangeAction(&document.getSequence(), ks.tick, sharpsOrFlats, isMinor));
}

void TransportBarComponent::updateDisplay()
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());

    auto bbt = document.getSequence().tickToBarBeatTick(tick);
    if (positionBarLabel.getCurrentTextEditor() == nullptr)
        positionBarLabel.setText(juce::String(bbt.bar).paddedLeft('0', 3), juce::dontSendNotification);
    if (positionBeatLabel.getCurrentTextEditor() == nullptr)
        positionBeatLabel.setText(juce::String(bbt.beat).paddedLeft('0', 2), juce::dontSendNotification);
    if (positionTickLabel.getCurrentTextEditor() == nullptr)
        positionTickLabel.setText(juce::String(bbt.tick).paddedLeft('0', 4), juce::dontSendNotification);

    auto ts = document.getSequence().getTimeSignatureAt(tick);
    if (timeSigNumLabel.getCurrentTextEditor() == nullptr)
        timeSigNumLabel.setText(juce::String(ts.numerator), juce::dontSendNotification);
    if (timeSigDenLabel.getCurrentTextEditor() == nullptr)
        timeSigDenLabel.setText(juce::String(ts.denominator), juce::dontSendNotification);

    if (keyValueLabel.getCurrentTextEditor() == nullptr)
    {
        if (document.getSequence().getKeySignatureChanges().empty())
            keyValueLabel.setText("-", juce::dontSendNotification);
        else
        {
            auto ks = document.getSequence().getKeySignatureAt(tick);
            keyValueLabel.setText(MidiSequence::keySignatureToString(ks.sharpsOrFlats, ks.isMinor),
                                  juce::dontSendNotification);
        }
    }

    if (tempoValueLabel.getCurrentTextEditor() == nullptr)
    {
        double tempo = document.getSequence().getTempoAt(tick);
        tempoValueLabel.setText(juce::String(tempo, 2), juce::dontSendNotification);
    }
}

void TransportBarComponent::tempoChanged()
{
    updateDisplay();
}

void TransportBarComponent::timelineMetadataChanged()
{
    updateDisplay();
}

void TransportBarComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(surface::bg2);
    g.fillRect(getLocalBounds());
    g.setColour(border::strong);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    auto drawInfoBox = [&](juce::Rectangle<int> b)
    {
        if (b.isEmpty())
            return;
        g.setColour(surface::surface);
        g.fillRoundedRectangle(b.toFloat(), radius::r2);
        g.setColour(border::normal);
        g.drawRoundedRectangle(b.toFloat().reduced(0.5f), radius::r2, 1.0f);
    };
    drawInfoBox(positionBoxBounds);
    drawInfoBox(infoBoxBounds);

    if (!infoBoxBounds.isEmpty())
    {
        g.setColour(border::soft);
        auto top = static_cast<float>(infoBoxBounds.getY()) + 1.0f;
        auto bottom = static_cast<float>(infoBoxBounds.getBottom()) - 1.0f;
        g.drawVerticalLine(infoDividerX1, top, bottom);
        g.drawVerticalLine(infoDividerX2, top, bottom);
    }
}

void TransportBarComponent::resized()
{
    auto toolbar = getLocalBounds();

    const int posW = 176;
    const int btnW = 172;
    const int tsW = 68;
    const int keyW = 60;
    const int tempoW = 96;
    const int infoW = tsW + keyW + tempoW;
    const int g1 = 20, g2 = 20;
    const int contentWidth = posW + g1 + btnW + g2 + infoW;

    auto content = toolbar.withSizeKeepingCentre(contentWidth, getHeight());

    const int boxH = 44;
    const int boxPadX = 8;
    const int boxPadTop = 5;
    const int headerH = 13;

    auto layoutSegment = [&](juce::Rectangle<int> segment, juce::Label& header, juce::Label& value)
    {
        auto inner = segment.reduced(boxPadX, 0);
        inner.removeFromTop(boxPadTop);
        header.setBounds(inner.removeFromTop(headerH));
        value.setBounds(inner.removeFromTop(boxH - boxPadTop - headerH));
    };

    auto posBox = content.removeFromLeft(posW).withSizeKeepingCentre(posW, boxH);
    positionBoxBounds = posBox;
    {
        auto inner = posBox.reduced(boxPadX, 0);
        inner.removeFromTop(boxPadTop);
        positionHeaderLabel.setBounds(inner.removeFromTop(headerH));
        auto valueRow = inner.removeFromTop(boxH - boxPadTop - headerH);

        using namespace calliope::theme;
        auto valueFont = font::mono(font::sizeDisplay).boldened();
        auto widthOf = [&](const char* s) { return juce::GlyphArrangement::getStringWidthInt(valueFont, s); };

        const int pad = 4;
        int barW = widthOf("000") + pad;
        int beatW = widthOf("00") + pad;
        int tickW = widthOf("0000") + pad;
        int dotW = widthOf(".");

        int groupW = barW + dotW + beatW + dotW + tickW;
        auto group = valueRow.withSizeKeepingCentre(groupW, valueRow.getHeight());
        positionBarLabel.setBounds(group.removeFromLeft(barW));
        positionDot1.setBounds(group.removeFromLeft(dotW));
        positionBeatLabel.setBounds(group.removeFromLeft(beatW));
        positionDot2.setBounds(group.removeFromLeft(dotW));
        positionTickLabel.setBounds(group.removeFromLeft(tickW));
    }
    content.removeFromLeft(g1);

    auto btnSection = content.removeFromLeft(btnW);
    auto btnArea = btnSection.withSizeKeepingCentre(btnW, 40);
    returnToStartButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    stopButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    playButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    loopButton.setBounds(btnArea.removeFromLeft(40));
    content.removeFromLeft(g2);

    auto infoBox = content.removeFromLeft(infoW).withSizeKeepingCentre(infoW, boxH);
    infoBoxBounds = infoBox;
    auto timeSeg = infoBox.removeFromLeft(tsW);
    auto keySeg = infoBox.removeFromLeft(keyW);
    auto tempoSeg = infoBox;
    infoDividerX1 = timeSeg.getRight();
    infoDividerX2 = keySeg.getRight();
    {
        auto inner = timeSeg.reduced(boxPadX, 0);
        inner.removeFromTop(boxPadTop);
        timeSigHeaderLabel.setBounds(inner.removeFromTop(headerH));
        auto valueRow = inner.removeFromTop(boxH - boxPadTop - headerH);

        using namespace calliope::theme;
        auto valueFont = font::mono(font::sizeDisplay).boldened();
        auto widthOf = [&](const char* s) { return juce::GlyphArrangement::getStringWidthInt(valueFont, s); };

        const int pad = 4;
        int numW = widthOf("00") + pad;
        int denW = widthOf("00") + pad;
        int slashW = widthOf("/");

        int groupW = numW + slashW + denW;
        auto group = valueRow.withSizeKeepingCentre(groupW, valueRow.getHeight());
        timeSigNumLabel.setBounds(group.removeFromLeft(numW));
        timeSigSlashLabel.setBounds(group.removeFromLeft(slashW));
        timeSigDenLabel.setBounds(group.removeFromLeft(denW));
    }
    layoutSegment(keySeg, keyHeaderLabel, keyValueLabel);
    layoutSegment(tempoSeg, tempoHeaderLabel, tempoValueLabel);
}
