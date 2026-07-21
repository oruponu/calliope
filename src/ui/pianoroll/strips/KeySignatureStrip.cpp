#include "ui/pianoroll/strips/KeySignatureStrip.h"
#include "ui/theme/Theme.h"
#include "undo/KeySignatureActions.h"
#include <algorithm>
#include <memory>

namespace
{
bool keySignatureTicksEqual(const std::vector<KeySignatureChange>& a, const std::vector<KeySignatureChange>& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](const KeySignatureChange& x, const KeySignatureChange& y) { return x.tick == y.tick; });
}
} // namespace

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
    isKeySigRangeSelecting = false;
    TimelineStrip::setSequence(seq);
}

bool KeySignatureStrip::hasSelection() const
{
    return !selectedKeySigIndices.empty();
}

void KeySignatureStrip::clearKeySignatureSelection()
{
    if (selectedKeySigIndices.empty())
        return;
    selectedKeySigIndices.clear();
    repaint();
}

void KeySignatureStrip::deleteSelectedKeySignatures()
{
    if (!sequence || selectedKeySigIndices.empty())
        return;

    auto before = sequence->getKeySignatureChanges();
    const int count = static_cast<int>(before.size());

    std::vector<KeySignatureChange> after;
    after.reserve(before.size());
    for (int i = 0; i < count; ++i)
        if (selectedKeySigIndices.count(i) == 0)
            after.push_back(before[static_cast<size_t>(i)]);

    if (after.size() == before.size())
        return;

    if (undoManager)
    {
        undoManager->beginNewTransaction("Delete Key Signature Changes");
        undoManager->perform(new KeySignatureDeleteAction(sequence, std::move(before), std::move(after)));
    }
    else
    {
        sequence->setKeySignatureChanges(std::move(after));
        sequence->notifyTimelineMetadataChanged();
    }

    clearKeySignatureSelection();
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
            bool selected = selectedKeySigIndices.count(static_cast<int>(i)) > 0;
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

    drawKeySignatureRangeSelection(g);

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
}

void KeySignatureStrip::drawKeySignatureRangeSelection(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!isKeySigRangeSelecting)
        return;

    int lo = std::min(keySigSelectStartX, keySigSelectCurrentX);
    int hi = std::max(keySigSelectStartX, keySigSelectCurrentX);
    if (hi <= lo)
        return;

    juce::Rectangle<float> band(static_cast<float>(lo), 0.0f, static_cast<float>(hi - lo),
                                static_cast<float>(getHeight()));
    g.setColour(track::sand.withAlpha(0.15f));
    g.fillRect(band);
    g.setColour(track::sand.withAlpha(0.6f));
    g.drawRect(band, 1.0f);
}

void KeySignatureStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.mods.isRightButtonDown())
        return;

    int ksIndex = hitTestKeySignaturePoint(e.x, e.y);
    if (ksIndex >= 0)
    {
        if (e.mods.isShiftDown())
        {
            if (onSelectionTaken)
                onSelectionTaken();
            if (selectedKeySigIndices.count(ksIndex) > 0)
                selectedKeySigIndices.erase(ksIndex);
            else
                selectedKeySigIndices.insert(ksIndex);
            repaint();
            return;
        }

        keySigDragBefore = sequence->getKeySignatureChanges();
        keySigDragIndex = ksIndex;
        isKeySigPointDragging = true;
        keySigDragMoved = false;
        keySigDragGrabOffset = geometry.xToTick(e.x) - keySigDragBefore[static_cast<size_t>(ksIndex)].tick;

        if (selectedKeySigIndices.count(ksIndex) > 0 && selectedKeySigIndices.size() > 1)
            keySigDragGroup.assign(selectedKeySigIndices.begin(), selectedKeySigIndices.end());
        else
            keySigDragGroup = {ksIndex};
        return;
    }

    if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
    {
        if (onSelectionTaken)
            onSelectionTaken();
        isKeySigRangeSelecting = true;
        keySigSelectStartX = e.x;
        keySigSelectCurrentX = e.x;
        keySigSelectBase = e.mods.isShiftDown() ? selectedKeySigIndices : std::set<int>{};
        if (!e.mods.isShiftDown())
            selectedKeySigIndices.clear();
        repaint();
        return;
    }
}

void KeySignatureStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (sequence == nullptr)
        return;

    if (isKeySigPointDragging)
    {
        if (keySigDragIndex < 0 || keySigDragIndex >= static_cast<int>(keySigDragBefore.size()))
            return;

        auto changes = sequence->buildKeySignatureChangesAfterMove(keySigDragBefore, keySigDragGroup, keySigDragIndex,
                                                                   geometry.xToTick(e.x) - keySigDragGrabOffset);
        if (!keySignatureTicksEqual(changes, keySigDragBefore))
            keySigDragMoved = true;
        sequence->setKeySignatureChanges(std::move(changes));
        repaint();
        return;
    }

    if (isKeySigRangeSelecting)
    {
        keySigSelectCurrentX = e.x;
        int lo = std::min(keySigSelectStartX, keySigSelectCurrentX);
        int hi = std::max(keySigSelectStartX, keySigSelectCurrentX);
        int tickLo = geometry.xToTick(lo);
        int tickHi = geometry.xToTick(hi);

        selectedKeySigIndices = keySigSelectBase;
        const auto& changes = sequence->getKeySignatureChanges();
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick >= tickLo && changes[static_cast<size_t>(i)].tick <= tickHi)
                selectedKeySigIndices.insert(i);

        repaint();
        return;
    }
}

void KeySignatureStrip::mouseUp(const juce::MouseEvent&)
{
    if (isKeySigPointDragging)
    {
        isKeySigPointDragging = false;
        int draggedIndex = keySigDragIndex;
        keySigDragIndex = -1;

        const auto& changes = sequence->getKeySignatureChanges();
        bool validIndex = draggedIndex >= 0 && draggedIndex < static_cast<int>(changes.size()) &&
                          draggedIndex < static_cast<int>(keySigDragBefore.size());
        bool movedFinal = validIndex && !keySignatureTicksEqual(changes, keySigDragBefore);

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
            selectedKeySigIndices = std::set<int>(keySigDragGroup.begin(), keySigDragGroup.end());
        }
        else if (validIndex && !keySigDragMoved)
        {
            const bool soleSelection =
                selectedKeySigIndices.size() == 1 && selectedKeySigIndices.count(draggedIndex) > 0;
            if (soleSelection && !isKeySigEditing)
            {
                const auto& ks = changes[static_cast<size_t>(draggedIndex)];
                openKeySignatureEditor(ks.tick, ks.sharpsOrFlats, ks.isMinor, false,
                                       keySignatureLabelRect(draggedIndex));
            }
            else if (!soleSelection)
            {
                if (onSelectionTaken)
                    onSelectionTaken();
                selectedKeySigIndices = {draggedIndex};
            }
        }
        else if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedKeySigIndices = {draggedIndex};
        }

        keySigDragBefore.clear();
        keySigDragGroup.clear();
        keySigDragMoved = false;
        repaint();
        return;
    }

    if (isKeySigRangeSelecting)
    {
        isKeySigRangeSelecting = false;
        keySigSelectBase.clear();
        repaint();
        return;
    }
}

void KeySignatureStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || isKeySigEditing)
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    if (hitTestKeySignaturePoint(e.x, e.y) >= 0)
        return;

    isKeySigRangeSelecting = false;
    keySigSelectBase.clear();

    int barStart = sequence->barStartToTick(sequence->tickToBarBeatTick(std::max(0, geometry.xToTick(e.x))).bar);

    const auto& changes = sequence->getKeySignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (changes[static_cast<size_t>(i)].tick == barStart)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedKeySigIndices = {i};
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
            selectedKeySigIndices = {i};
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
