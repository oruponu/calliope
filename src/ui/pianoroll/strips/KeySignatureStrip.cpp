#include "ui/pianoroll/strips/KeySignatureStrip.h"
#include "ui/theme/Theme.h"

KeySignatureStrip::KeySignatureStrip(const TimelineGeometry& geometryRef) : TimelineStrip(geometryRef, "Key") {}

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

    const auto& ksChanges = sequence->getKeySignatureChanges();
    if (ksChanges.empty())
    {
        g.setColour(track::sand);
        g.setFont(font::sans(font::sizeSM));
        g.drawText("C", viewLeftX + labelWidth() + 4, 0, 40, getHeight(), juce::Justification::centredLeft);
    }
    else
    {
        juce::Colour ksColour = track::sand;

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
                g.setColour(ksColour);
                g.setFont(font::sans(font::sizeSM));
                juce::String labelText =
                    juce::String(MidiSequence::keySignatureToString(ksChanges[i].sharpsOrFlats, ksChanges[i].isMinor));
                int textX = (i == 0 && ksChanges[i].tick == 0) ? viewLeftX + labelWidth() + 4 : x + 4;
                g.drawText(labelText, textX, 0, 60, getHeight(), juce::Justification::centredLeft);
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
