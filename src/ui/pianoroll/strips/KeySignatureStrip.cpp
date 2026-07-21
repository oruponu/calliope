#include "ui/pianoroll/strips/KeySignatureStrip.h"
#include "ui/theme/Theme.h"
#include "undo/KeySignatureActions.h"
#include <algorithm>
#include <memory>

KeySignatureStrip::KeySignatureStrip(const TimelineGeometry& geometryRef) : TimelineStrip(geometryRef, "Key") {}

KeySignatureStrip::~KeySignatureStrip()
{
    closeKeySignatureEditor();
}

void KeySignatureStrip::setUndoManager(juce::UndoManager* um)
{
    undoManager = um;
}

void KeySignatureStrip::setSequence(MidiSequence* seq)
{
    closeKeySignatureEditor();
    clearKeySignatureSelection();
    TimelineStrip::setSequence(seq);
}

bool KeySignatureStrip::hasSelection() const
{
    return selectedKeySigIndex >= 0;
}

void KeySignatureStrip::clearKeySignatureSelection()
{
    if (selectedKeySigIndex < 0)
        return;
    selectedKeySigIndex = -1;
    repaint();
}

juce::Rectangle<int> KeySignatureStrip::keySignatureLabelRect(int index) const
{
    const auto& changes = sequence->getKeySignatureChanges();
    const auto& ks = changes[static_cast<size_t>(index)];
    int x = geometry.tickToX(ks.tick);
    int textX = (index == 0 && ks.tick == 0) ? viewLeftX + labelWidth() + 4 : x + 4;
    return {textX, 0, 40, getHeight()};
}

int KeySignatureStrip::hitTestKeySignaturePoint(int x, int y) const
{
    if (!sequence)
        return -1;

    if (y < 0 || y >= getHeight())
        return -1;

    if (x < viewLeftX + labelWidth())
        return -1;

    const auto& changes = sequence->getKeySignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (geometry.tickToX(changes[static_cast<size_t>(i)].tick) + 4 < viewLeftX - 40)
            continue;
        if (keySignatureLabelRect(i).contains(x, y))
            return i;
    }
    return -1;
}

void KeySignatureStrip::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(surface::surface2);
    g.fillRect(viewLeftX, 0, getWidth() - viewLeftX, getHeight());

    g.saveState();
    g.reduceClipRegion(viewLeftX + labelWidth(), 0, getWidth(), getHeight());

    drawTrackGridLines(g, visibleLeft, visibleRight, 0.0f, static_cast<float>(getHeight()));

    juce::Colour ksColour = track::sand;
    const auto& ksChanges = sequence->getKeySignatureChanges();

    for (size_t i = 0; i < ksChanges.size(); ++i)
    {
        int x = geometry.tickToX(ksChanges[i].tick);

        if (x > visibleRight)
            break;

        int nextX = (i + 1 < ksChanges.size()) ? geometry.tickToX(ksChanges[i + 1].tick)
                                               : geometry.tickToX(geometry.xToTick(getWidth()));
        if (nextX < visibleLeft)
            continue;

        if (i > 0 && x >= visibleLeft && x <= visibleRight)
        {
            g.setColour(ksColour.withAlpha(0.6f));
            g.drawVerticalLine(x, 2.0f, static_cast<float>(getHeight() - 2));
        }

        if (x + 4 >= visibleLeft - 40 && x <= visibleRight)
        {
            bool selected = selectedKeySigIndex == static_cast<int>(i);
            bool editingThis = isKeySigEditing && !keySigEditIsNew && ksChanges[i].tick == keySigEditTick;
            auto labelRect = keySignatureLabelRect(static_cast<int>(i));
            if (selected || editingThis)
            {
                g.setColour(surface::selection);
                g.fillRoundedRectangle(labelRect.reduced(0, 2).toFloat(), radius::r1);
            }
            g.setColour(selected || editingThis ? ksColour.brighter(0.5f) : ksColour);
            g.setFont(font::sans(font::sizeSM));
            int sf = editingThis ? keySigDraftSharpsOrFlats : ksChanges[i].sharpsOrFlats;
            bool minor = editingThis ? keySigDraftIsMinor : ksChanges[i].isMinor;
            juce::String labelText = juce::String(MidiSequence::keySignatureToString(sf, minor));
            g.drawText(labelText, labelRect, juce::Justification::centredLeft);
        }
    }

    if (isKeySigEditing && keySigEditIsNew)
    {
        juce::Colour draftColour = ksColour.withAlpha(0.6f);
        int x = geometry.tickToX(keySigEditTick);
        if (x >= visibleLeft && x <= visibleRight)
        {
            g.setColour(draftColour.withAlpha(0.4f));
            g.drawVerticalLine(x, 2.0f, static_cast<float>(getHeight() - 2));
        }
        int textX = std::max(x + 4, viewLeftX + labelWidth() + 4);
        g.setColour(draftColour);
        g.setFont(font::sans(font::sizeSM));
        g.drawText(juce::String(MidiSequence::keySignatureToString(keySigDraftSharpsOrFlats, keySigDraftIsMinor)),
                   textX, 0, 40, getHeight(), juce::Justification::centredLeft);
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
}

void KeySignatureStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.mods.isRightButtonDown())
        return;

    int ksIndex = hitTestKeySignaturePoint(e.x, e.y);
    if (ksIndex < 0)
        return;

    keySigDragBefore = sequence->getKeySignatureChanges();
    keySigDragIndex = ksIndex;
    isKeySigPointDragging = true;
    keySigDragMoved = false;
    keySigDragGrabOffset = geometry.xToTick(e.x) - keySigDragBefore[static_cast<size_t>(ksIndex)].tick;
}

void KeySignatureStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (sequence == nullptr || !isKeySigPointDragging)
        return;
    if (keySigDragIndex < 0 || keySigDragIndex >= static_cast<int>(keySigDragBefore.size()))
        return;

    auto changes = sequence->buildKeySignatureChangesAfterMove(keySigDragBefore, keySigDragIndex,
                                                               geometry.xToTick(e.x) - keySigDragGrabOffset);
    if (changes[static_cast<size_t>(keySigDragIndex)].tick !=
        keySigDragBefore[static_cast<size_t>(keySigDragIndex)].tick)
        keySigDragMoved = true;
    sequence->setKeySignatureChanges(std::move(changes));
    repaint();
}

void KeySignatureStrip::mouseUp(const juce::MouseEvent&)
{
    if (!isKeySigPointDragging)
        return;

    isKeySigPointDragging = false;
    int draggedIndex = keySigDragIndex;
    keySigDragIndex = -1;

    const auto& changes = sequence->getKeySignatureChanges();
    bool validIndex = draggedIndex >= 0 && draggedIndex < static_cast<int>(changes.size()) &&
                      draggedIndex < static_cast<int>(keySigDragBefore.size());
    bool movedFinal = validIndex && changes[static_cast<size_t>(draggedIndex)].tick !=
                                        keySigDragBefore[static_cast<size_t>(draggedIndex)].tick;

    if (movedFinal)
    {
        if (undoManager)
        {
            undoManager->beginNewTransaction("Move Key Signature Change");
            undoManager->perform(new KeySignatureMoveAction(sequence, keySigDragBefore, changes));
        }
        else
        {
            sequence->notifyTimelineMetadataChanged();
        }
        if (onSelectionTaken)
            onSelectionTaken();
        selectedKeySigIndex = draggedIndex;
    }
    else if (validIndex && !keySigDragMoved && selectedKeySigIndex == draggedIndex && !isKeySigEditing)
    {
        const auto& ks = changes[static_cast<size_t>(draggedIndex)];
        openKeySignatureEditor(ks.tick, ks.sharpsOrFlats, ks.isMinor, false, keySignatureLabelRect(draggedIndex));
    }
    else if (validIndex)
    {
        if (onSelectionTaken)
            onSelectionTaken();
        selectedKeySigIndex = draggedIndex;
    }

    keySigDragBefore.clear();
    keySigDragMoved = false;
    repaint();
}

void KeySignatureStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || isKeySigEditing)
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    if (hitTestKeySignaturePoint(e.x, e.y) >= 0)
        return;

    int barStart = sequence->barStartToTick(sequence->tickToBarBeatTick(std::max(0, geometry.xToTick(e.x))).bar);

    const auto& changes = sequence->getKeySignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (changes[static_cast<size_t>(i)].tick == barStart)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedKeySigIndex = i;
            repaint();
            openKeySignatureEditor(barStart, changes[static_cast<size_t>(i)].sharpsOrFlats,
                                   changes[static_cast<size_t>(i)].isMinor, false, keySignatureLabelRect(i));
            return;
        }
    }

    if (onSelectionTaken)
        onSelectionTaken();
    clearKeySignatureSelection();
    auto effective = sequence->getKeySignatureAt(barStart);
    juce::Rectangle<int> anchor{geometry.tickToX(barStart), 0, 40, getHeight()};
    openKeySignatureEditor(barStart, effective.sharpsOrFlats, effective.isMinor, true, anchor);
}

void KeySignatureStrip::openKeySignatureEditor(int tick, int sharpsOrFlats, bool isMinor, bool isNew,
                                               juce::Rectangle<int> anchorInLocal)
{
    anchorInLocal.setX(std::max(anchorInLocal.getX(), viewLeftX + labelWidth()));

    isKeySigEditing = true;
    keySigEditTick = tick;
    keySigDraftSharpsOrFlats = sharpsOrFlats;
    keySigDraftIsMinor = isMinor;
    keySigEditIsNew = isNew;

    auto content = std::make_unique<KeySignatureEditor>(sharpsOrFlats, isMinor, isNew);
    keySigEditor = content.get();
    content->onDraftChanged = [this](int sf, bool minor)
    {
        keySigDraftSharpsOrFlats = sf;
        keySigDraftIsMinor = minor;
        repaint();
    };
    content->onCommit = [this](int sf, bool minor) { commitKeySignatureEdit(sf, minor); };
    content->onCancel = [this]() { cancelKeySignatureEdit(); };

    auto& box = juce::CallOutBox::launchAsynchronously(std::move(content), localAreaToGlobal(anchorInLocal), nullptr);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    keySigCallout = &box;
    repaint();
}

void KeySignatureStrip::commitKeySignatureEdit(int sharpsOrFlats, bool isMinor)
{
    isKeySigEditing = false;
    keySigEditor = nullptr;
    keySigCallout = nullptr;

    if (!sequence)
        return;

    const auto& existing = sequence->getKeySignatureChanges();
    auto atTick = std::ranges::find(existing, keySigEditTick, &KeySignatureChange::tick);
    if (atTick != existing.end() && sharpsOrFlats == atTick->sharpsOrFlats && isMinor == atTick->isMinor)
    {
        repaint();
        return;
    }

    if (undoManager)
    {
        undoManager->beginNewTransaction(keySigEditIsNew ? "Add Key Signature Change" : "Edit Key Signature Change");
        undoManager->perform(new KeySignatureChangeAction(sequence, keySigEditTick, sharpsOrFlats, isMinor));
    }
    else
    {
        sequence->addKeySignatureChange(keySigEditTick, sharpsOrFlats, isMinor);
        sequence->notifyTimelineMetadataChanged();
    }

    const auto& changes = sequence->getKeySignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[static_cast<size_t>(i)].tick == keySigEditTick)
            selectedKeySigIndex = i;
    repaint();
}

void KeySignatureStrip::cancelKeySignatureEdit()
{
    isKeySigEditing = false;
    keySigEditor = nullptr;
    keySigCallout = nullptr;
    repaint();
}

void KeySignatureStrip::closeKeySignatureEditor()
{
    if (keySigEditor != nullptr)
        keySigEditor->abandon();
    if (keySigCallout != nullptr)
        keySigCallout->dismiss();

    isKeySigEditing = false;
    keySigEditor = nullptr;
    keySigCallout = nullptr;
}
