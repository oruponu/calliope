#include "ui/pianoroll/strips/ChordStrip.h"
#include "ui/theme/Theme.h"
#include "undo/ChordActions.h"
#include <algorithm>
#include <memory>

ChordStrip::ChordStrip(const TimelineGeometry& geometryRef) : TimelineStrip(geometryRef, "Chord") {}

ChordStrip::~ChordStrip()
{
    closeChordEditor();
}

void ChordStrip::setUndoManager(juce::UndoManager* um)
{
    undoManager = um;
}

void ChordStrip::setSequence(MidiSequence* seq)
{
    closeChordEditor();
    clearChordSelection();
    TimelineStrip::setSequence(seq);
}

void ChordStrip::clearChordSelection()
{
    if (selectedChordIndex < 0)
        return;
    selectedChordIndex = -1;
    repaint();
}

juce::Rectangle<int> ChordStrip::chordSpanRect(int index) const
{
    if (!sequence)
        return {};

    const auto& changes = sequence->getChordChanges();
    if (index < 0 || index >= static_cast<int>(changes.size()))
        return {};
    if (MidiSequence::chordToString(changes[static_cast<size_t>(index)]).empty())
        return {};

    int x = geometry.tickToX(changes[static_cast<size_t>(index)].tick);
    int nextX = (index + 1 < static_cast<int>(changes.size()))
                    ? geometry.tickToX(changes[static_cast<size_t>(index + 1)].tick)
                    : geometry.tickToX(geometry.xToTick(getWidth()));
    return {x, spanTop, nextX - x, spanHeight()};
}

juce::Rectangle<int> ChordStrip::chordDraftSpanRect() const
{
    if (!sequence)
        return {};

    int x = geometry.tickToX(chordEditTick);
    int nextX = geometry.tickToX(geometry.xToTick(getWidth()));
    for (const auto& cc : sequence->getChordChanges())
    {
        if (cc.tick > chordEditTick)
        {
            nextX = geometry.tickToX(cc.tick);
            break;
        }
    }
    return {x, spanTop, nextX - x, spanHeight()};
}

int ChordStrip::hitTestChordSpan(int x, int y) const
{
    if (!sequence)
        return -1;

    if (x < viewLeftX + labelWidth())
        return -1;

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        auto spanRect = chordSpanRect(i);
        if (!spanRect.isEmpty() && spanRect.contains(x, y))
            return i;
    }
    return -1;
}

void ChordStrip::paint(juce::Graphics& g)
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

    juce::Colour chordColour = track::violet;
    const auto& chordChanges = sequence->getChordChanges();

    for (int i = 0; i < static_cast<int>(chordChanges.size()); ++i)
    {
        auto spanRect = chordSpanRect(i);
        if (spanRect.isEmpty())
            continue;
        if (spanRect.getX() > visibleRight || spanRect.getRight() < visibleLeft)
            continue;

        bool selected = (selectedChordIndex == i);
        bool editingThis =
            isChordEditing && !chordEditIsNew && chordChanges[static_cast<size_t>(i)].tick == chordEditTick;
        bool highlighted = selected || editingThis;

        if (highlighted)
        {
            g.setColour(surface::selection);
            g.fillRoundedRectangle(spanRect.toFloat(), radius::r1);
        }
        else
        {
            g.setColour(chordColour.withAlpha(0.15f));
            g.fillRect(spanRect);
        }
        g.setColour(chordColour.withAlpha(highlighted ? 0.8f : 0.5f));
        g.drawRect(spanRect, 1);

        int textX = spanRect.getX() + 4;
        int textWidth = spanRect.getRight() - textX - 2;
        if (textWidth > 8)
        {
            ChordChange displayed = chordChanges[static_cast<size_t>(i)];
            if (editingThis)
            {
                displayed.chordRoot = chordDraftRoot;
                displayed.chordType = chordDraftType;
                displayed.bassRoot = chordDraftBassRoot;
            }
            g.setColour(highlighted ? chordColour.brighter(0.5f) : chordColour);
            g.setFont(font::sans(font::sizeSM));
            g.drawText(juce::String(MidiSequence::chordToString(displayed)), textX, 0, textWidth, getHeight(),
                       juce::Justification::centredLeft);
        }
    }

    if (isChordEditing && chordEditIsNew)
    {
        auto draftRect = chordDraftSpanRect();
        if (!draftRect.isEmpty() && draftRect.getX() <= visibleRight && draftRect.getRight() >= visibleLeft)
        {
            g.setColour(chordColour.withAlpha(0.1f));
            g.fillRect(draftRect);
            g.setColour(chordColour.withAlpha(0.35f));
            g.drawRect(draftRect, 1);

            int textX = draftRect.getX() + 4;
            int textWidth = draftRect.getRight() - textX - 2;
            if (textWidth > 8)
            {
                ChordChange draft{chordEditTick, chordDraftRoot, chordDraftType, chordDraftBassRoot,
                                  MidiSequence::chordNone};
                g.setColour(chordColour.withAlpha(0.6f));
                g.setFont(font::sans(font::sizeSM));
                g.drawText(juce::String(MidiSequence::chordToString(draft)), textX, 0, textWidth, getHeight(),
                           juce::Justification::centredLeft);
            }
        }
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

void ChordStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.mods.isRightButtonDown())
        return;

    int index = hitTestChordSpan(e.x, e.y);
    if (index < 0)
        return;

    if (onSelectionTaken)
        onSelectionTaken();
    selectedChordIndex = index;
    repaint();
}

void ChordStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || isChordEditing)
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    const int floorTick = geometry.floorTickToGrid(std::max(0, geometry.xToTick(e.x)));
    const int grid = geometry.gridTicks();

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        const auto& cc = changes[static_cast<size_t>(i)];
        if (cc.tick < floorTick || cc.tick >= floorTick + grid)
            continue;
        if (MidiSequence::chordToString(cc).empty())
            continue;

        if (onSelectionTaken)
            onSelectionTaken();
        selectedChordIndex = i;
        repaint();
        openChordEditor(cc.tick, cc.chordRoot, cc.chordType, cc.bassRoot, false,
                        {chordSpanRect(i).getX() + 4, 0, 40, getHeight()});
        return;
    }

    if (onSelectionTaken)
        onSelectionTaken();
    clearChordSelection();

    int root = 0x31;
    int type = 0;
    int bassRoot = MidiSequence::chordNone;
    for (int i = static_cast<int>(changes.size()) - 1; i >= 0; --i)
    {
        const auto& cc = changes[static_cast<size_t>(i)];
        if (cc.tick >= floorTick)
            continue;
        if (MidiSequence::chordToString(cc).empty())
            continue;
        root = cc.chordRoot;
        type = cc.chordType;
        bassRoot = cc.bassRoot;
        break;
    }

    openChordEditor(floorTick, root, type, bassRoot, true, {geometry.tickToX(floorTick) + 4, 0, 40, getHeight()});
}

void ChordStrip::openChordEditor(int tick, int chordRoot, int chordType, int bassRoot, bool isNew,
                                 juce::Rectangle<int> anchorInLocal)
{
    anchorInLocal.setX(std::max(anchorInLocal.getX(), viewLeftX + labelWidth()));

    chordRoot = MidiSequence::normalizeChordRoot(chordRoot);
    chordType = MidiSequence::normalizeChordType(chordType);
    bassRoot = MidiSequence::normalizeChordBassRoot(bassRoot);

    isChordEditing = true;
    chordEditTick = tick;
    chordDraftRoot = chordRoot;
    chordDraftType = chordType;
    chordDraftBassRoot = bassRoot;
    chordEditIsNew = isNew;
    chordEditSpelling = MidiSequence::chordSpellingForKeySignature(sequence->getKeySignatureAt(tick).sharpsOrFlats);

    auto content = std::make_unique<ChordEditor>(chordRoot, chordType, bassRoot, chordEditSpelling, isNew);
    chordEditor = content.get();
    content->onDraftChanged = [this](int root, int type, int bass)
    {
        chordDraftRoot = root;
        chordDraftType = type;
        chordDraftBassRoot = bass;
        repaint();
    };
    content->onCommit = [this](int root, int type, int bass) { commitChordEdit(root, type, bass); };
    content->onCancel = [this]() { cancelChordEdit(); };

    auto& box = juce::CallOutBox::launchAsynchronously(std::move(content), localAreaToGlobal(anchorInLocal), nullptr);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    chordCallout = &box;
    repaint();
}

void ChordStrip::commitChordEdit(int chordRoot, int chordType, int bassRoot)
{
    isChordEditing = false;
    chordEditor = nullptr;
    chordCallout = nullptr;

    if (!sequence)
        return;

    const int bassType = (bassRoot == MidiSequence::chordNone) ? MidiSequence::chordNone : chordType;

    const auto& existing = sequence->getChordChanges();
    auto atTick = std::ranges::find(existing, chordEditTick, &ChordChange::tick);
    if (atTick != existing.end() && atTick->chordRoot == chordRoot && atTick->chordType == chordType &&
        atTick->bassRoot == bassRoot)
    {
        repaint();
        return;
    }

    if (undoManager)
    {
        undoManager->beginNewTransaction(chordEditIsNew ? "Add Chord" : "Edit Chord");
        undoManager->perform(new ChordChangeAction(sequence, chordEditTick, chordRoot, chordType, bassRoot, bassType));
    }
    else
    {
        sequence->addChordChange(chordEditTick, chordRoot, chordType, bassRoot, bassType);
        sequence->notifyTimelineMetadataChanged();
    }

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[static_cast<size_t>(i)].tick == chordEditTick)
            selectedChordIndex = i;
    repaint();
}

void ChordStrip::cancelChordEdit()
{
    isChordEditing = false;
    chordEditor = nullptr;
    chordCallout = nullptr;
    repaint();
}

void ChordStrip::closeChordEditor()
{
    if (chordEditor != nullptr)
        chordEditor->abandon();
    if (chordCallout != nullptr)
        chordCallout->dismiss();

    isChordEditing = false;
    chordEditor = nullptr;
    chordCallout = nullptr;
}
