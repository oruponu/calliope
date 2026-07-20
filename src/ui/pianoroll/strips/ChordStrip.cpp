#include "ui/pianoroll/strips/ChordStrip.h"
#include "ui/theme/Theme.h"

ChordStrip::ChordStrip(const TimelineGeometry& geometryRef) : TimelineStrip(geometryRef, "Chord") {}

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

    const auto& chordChanges = sequence->getChordChanges();
    if (!chordChanges.empty())
    {
        juce::Colour chordColour = track::violet;
        int barY = 3;
        int barH = getHeight() - 6;

        for (size_t i = 0; i < chordChanges.size(); ++i)
        {
            auto labelText = MidiSequence::chordToString(chordChanges[i]);
            if (labelText.empty())
                continue;

            int x = geometry.tickToX(chordChanges[i].tick);
            int nextX = (i + 1 < chordChanges.size()) ? geometry.tickToX(chordChanges[i + 1].tick)
                                                      : geometry.tickToX(geometry.xToTick(getWidth()));

            if (x > visibleRight || nextX < visibleLeft)
                continue;

            g.setColour(chordColour.withAlpha(0.15f));
            g.fillRect(x, barY, nextX - x, barH);
            g.setColour(chordColour.withAlpha(0.5f));
            g.drawRect(x, barY, nextX - x, barH, 1);

            int textX = x + 4;
            int textWidth = nextX - textX - 2;
            if (textWidth > 8)
            {
                g.setColour(chordColour);
                g.setFont(font::sans(font::sizeSM));
                g.drawText(juce::String(labelText), textX, 0, textWidth, getHeight(), juce::Justification::centredLeft);
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
