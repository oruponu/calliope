#include "ui/pianoroll/strips/KeySignatureStrip.h"
#include "edit/KeySignatureEdits.h"
#include "notation/KeySignatureName.h"
#include "ui/theme/Theme.h"
#include "undo/ReplaceListAction.h"
#include <algorithm>
#include <memory>
#include <utility>

namespace
{
bool keySignatureTicksEqual(const std::vector<KeySignatureChange>& a, const std::vector<KeySignatureChange>& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](const KeySignatureChange& x, const KeySignatureChange& y) { return x.tick == y.tick; });
}
} // namespace

KeySignatureStrip::KeySignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                                     juce::UndoManager& undoManagerRef)
    : TimelineStrip(geometryRef, "Key"), clipboard(clipboardRef), undoManager(undoManagerRef)
{
}

void KeySignatureStrip::setSequence(MidiSequence* seq)
{
    editSession.close();
    clearKeySignatureSelection();
    isKeySigRangeSelecting = false;
    TimelineStrip::setSequence(seq);
}

bool KeySignatureStrip::hasSelection() const
{
    return !selection.isEmpty();
}

void KeySignatureStrip::clearKeySignatureSelection()
{
    if (selection.isEmpty())
        return;
    selection.clear();
    repaint();
}

void KeySignatureStrip::deleteSelectedKeySignatures()
{
    deleteSelectedKeySignaturesImpl("Delete Key Signature Changes");
}

void KeySignatureStrip::deleteSelectedKeySignaturesImpl(const juce::String& transactionName)
{
    if (!sequence || selection.isEmpty())
        return;

    auto before = sequence->getKeySignatureChanges();
    const int count = static_cast<int>(before.size());

    std::vector<KeySignatureChange> after;
    after.reserve(before.size());
    for (int i = 0; i < count; ++i)
        if (!selection.contains(i))
            after.push_back(before[static_cast<size_t>(i)]);

    if (after.size() == before.size())
        return;

    performReplaceList(undoManager, sequence, transactionName, std::move(before), std::move(after));

    clearKeySignatureSelection();
    repaint();
}

void KeySignatureStrip::copySelectedKeySignatures()
{
    if (!sequence || selection.isEmpty())
        return;

    const auto& changes = sequence->getKeySignatureChanges();
    const int count = static_cast<int>(changes.size());

    std::vector<RelativeKeySignature> items;
    for (int i : selection.indices())
        if (i >= 0 && i < count)
            items.push_back({sequence->getTimeline().tickToBarBeatTick(changes[i].tick).bar, changes[i].sharpsOrFlats,
                             changes[i].isMinor});

    if (items.empty())
        return;

    const int firstBar = items.front().barOffset;
    for (auto& item : items)
        item.barOffset -= firstBar;

    clipboard.setKeySignatures(std::move(items));
}

void KeySignatureStrip::cutSelectedKeySignatures()
{
    if (!sequence || selection.isEmpty())
        return;

    copySelectedKeySignatures();
    deleteSelectedKeySignaturesImpl("Cut Key Signature Changes");
}

void KeySignatureStrip::pasteKeySignatures(int atTick)
{
    if (!sequence || !clipboard.hasKeySignatures())
        return;

    const int anchorBar = sequence->getTimeline().tickToBarBeatTick(std::max(0, atTick)).bar;
    auto before = sequence->getKeySignatureChanges();
    auto after = before;

    std::vector<int> pastedTicks;
    for (const auto& item : clipboard.getKeySignatures())
    {
        const int tick = sequence->getTimeline().barStartToTick(anchorBar + item.barOffset);
        pastedTicks.push_back(tick);
        auto it = std::ranges::find(after, tick, &KeySignatureChange::tick);
        if (it != after.end())
        {
            it->sharpsOrFlats = item.sharpsOrFlats;
            it->isMinor = item.isMinor;
        }
        else
            after.push_back({tick, item.sharpsOrFlats, item.isMinor});
    }
    std::ranges::sort(after, {}, &KeySignatureChange::tick);

    const bool changed = (after != before);
    if (changed)
    {
        performReplaceList(undoManager, sequence, "Paste Key Signature Changes", std::move(before), after);
    }

    selection.selectTicks(sequence->getKeySignatureChanges(), pastedTicks);

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
    const auto* draft = editSession.current();

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
            bool selected = selection.contains(static_cast<int>(i));
            bool editingThis = draft != nullptr && !draft->isNew && ksChanges[i].tick == draft->tick;
            auto labelRect = keySignatureLabelRect(static_cast<int>(i));
            if (selected || editingThis)
            {
                g.setColour(surface::selection);
                g.fillRoundedRectangle(labelRect.reduced(0, 2).toFloat(), radius::r1);
            }
            g.setColour(selected || editingThis ? ksColour.brighter(0.5f) : ksColour);
            g.setFont(font::sans(font::sizeSM));
            int sf = editingThis ? draft->sharpsOrFlats : ksChanges[i].sharpsOrFlats;
            bool minor = editingThis ? draft->isMinor : ksChanges[i].isMinor;
            juce::String labelText = juce::String(KeySignatureName::toString(sf, minor));
            g.drawText(labelText, labelRect, juce::Justification::centredLeft);
        }
    }

    if (draft != nullptr && draft->isNew)
    {
        juce::Colour draftColour = ksColour.withAlpha(0.6f);
        int x = geometry.tickToX(draft->tick);
        if (x >= visibleLeft && x <= visibleRight)
        {
            g.setColour(draftColour.withAlpha(0.4f));
            g.drawVerticalLine(x, 2.0f, static_cast<float>(getHeight() - 2));
        }
        int textX = std::max(x + 4, viewLeftX + labelWidth() + 4);
        g.setColour(draftColour);
        g.setFont(font::sans(font::sizeSM));
        g.drawText(juce::String(KeySignatureName::toString(draft->sharpsOrFlats, draft->isMinor)), textX, 0, 40,
                   getHeight(), juce::Justification::centredLeft);
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    if (isKeySigRangeSelecting)
        drawRangeBand(g, rangeSelect, track::sand.withAlpha(0.15f), track::sand.withAlpha(0.6f));

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
    if (ksIndex >= 0)
    {
        if (e.mods.isShiftDown())
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.toggle(ksIndex);
            repaint();
            return;
        }

        keySigDragBefore = sequence->getKeySignatureChanges();
        keySigDragIndex = ksIndex;
        isKeySigPointDragging = true;
        keySigDragMoved = false;
        keySigDragGrabOffset = geometry.xToTick(e.x) - keySigDragBefore[static_cast<size_t>(ksIndex)].tick;

        keySigDragGroup = selection.dragGroup(ksIndex);
        return;
    }

    if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
    {
        if (onSelectionTaken)
            onSelectionTaken();
        isKeySigRangeSelecting = true;
        rangeSelect = {e.x, e.x, e.mods.isShiftDown() ? selection.indices() : std::set<int>{}};
        if (!e.mods.isShiftDown())
            selection.clear();
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

        auto changes =
            KeySignatureEdits::afterMove(keySigDragBefore, keySigDragGroup, keySigDragIndex,
                                         geometry.xToTick(e.x) - keySigDragGrabOffset, sequence->getTimeline());
        if (!keySignatureTicksEqual(changes, keySigDragBefore))
            keySigDragMoved = true;
        sequence->setKeySignatureChanges(std::move(changes));
        repaint();
        return;
    }

    if (isKeySigRangeSelecting)
    {
        rangeSelect.currentX = e.x;
        const auto& changes = sequence->getKeySignatureChanges();
        selection.assign(rangeSelect.selectionFor(static_cast<int>(changes.size()), geometry,
                                                  [&changes](int i, int tickLo, int tickHi)
                                                  {
                                                      const int tick = changes[static_cast<size_t>(i)].tick;
                                                      return tick >= tickLo && tick <= tickHi;
                                                  }));

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
            performReplaceList(undoManager, sequence, "Move Key Signature Change", keySigDragBefore, changes);
            if (onSelectionTaken)
                onSelectionTaken();
            selection.assign(std::set<int>(keySigDragGroup.begin(), keySigDragGroup.end()));
        }
        else if (validIndex && !keySigDragMoved)
        {
            const bool soleSelection = selection.isSole(draggedIndex);
            if (soleSelection && !editSession.isOpen())
            {
                const auto& ks = changes[static_cast<size_t>(draggedIndex)];
                openKeySignatureEditor(ks.tick, ks.sharpsOrFlats, ks.isMinor, false,
                                       keySignatureLabelRect(draggedIndex));
            }
            else if (!soleSelection)
            {
                if (onSelectionTaken)
                    onSelectionTaken();
                selection.selectOnly(draggedIndex);
            }
        }
        else if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(draggedIndex);
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
        rangeSelect = {};
        repaint();
        return;
    }
}

void KeySignatureStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || editSession.isOpen())
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    if (hitTestKeySignaturePoint(e.x, e.y) >= 0)
        return;

    isKeySigRangeSelecting = false;
    rangeSelect = {};

    int barStart = sequence->getTimeline().barStartToTick(
        sequence->getTimeline().tickToBarBeatTick(std::max(0, geometry.xToTick(e.x))).bar);

    const auto& changes = sequence->getKeySignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (changes[static_cast<size_t>(i)].tick == barStart)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(i);
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

    auto content = std::make_unique<KeySignatureEditor>(sharpsOrFlats, isMinor, isNew);
    content->onDraftChanged = [this](int sf, bool minor)
    {
        if (auto* draft = editSession.current())
        {
            draft->sharpsOrFlats = sf;
            draft->isMinor = minor;
        }
        repaint();
    };
    content->onCommit = [this](int sf, bool minor) { commitKeySignatureEdit(sf, minor); };
    content->onCancel = [this]() { cancelKeySignatureEdit(); };

    editSession.open({tick, sharpsOrFlats, isMinor, isNew}, std::move(content), localAreaToGlobal(anchorInLocal));
    repaint();
}

void KeySignatureStrip::commitKeySignatureEdit(int sharpsOrFlats, bool isMinor)
{
    const auto draft = editSession.finish();

    if (!sequence)
        return;

    const auto& existing = sequence->getKeySignatureChanges();
    auto atTick = std::ranges::find(existing, draft.tick, &KeySignatureChange::tick);
    if (atTick != existing.end() && sharpsOrFlats == atTick->sharpsOrFlats && isMinor == atTick->isMinor)
    {
        repaint();
        return;
    }

    auto before = sequence->getKeySignatureChanges();
    auto after = before;
    KeySignatureEdits::add(after, draft.tick, sharpsOrFlats, isMinor);
    performReplaceList(undoManager, sequence, draft.isNew ? "Add Key Signature Change" : "Edit Key Signature Change",
                       std::move(before), std::move(after));

    const auto& changes = sequence->getKeySignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[static_cast<size_t>(i)].tick == draft.tick)
            selection.selectOnly(i);
    repaint();
}

void KeySignatureStrip::cancelKeySignatureEdit()
{
    editSession.finish();
    repaint();
}
