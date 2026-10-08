#include "audio/ClickVoice.h"
#include <cmath>
#include <numbers>

namespace
{
constexpr double decayTimeConstantsPerClick = 5.0;
} // namespace

ClickVoice::ClickVoice()
{
    prepare(44100.0);
}

void ClickVoice::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    lengthSamples = static_cast<int>(std::lround(lengthSeconds * sampleRate));
    position = lengthSamples;
}

void ClickVoice::trigger(bool accent)
{
    frequency = accent ? accentFrequency : plainFrequency;
    position = 0;
}

void ClickVoice::render(float* const* channels, int numChannels, int startSample, int numSamples, float gain)
{
    const double tau = lengthSamples / decayTimeConstantsPerClick;
    const double endLevel = std::exp(-decayTimeConstantsPerClick);
    const double phasePerSample = 2.0 * std::numbers::pi * frequency / sampleRate;

    for (int i = startSample; i < startSample + numSamples && position < lengthSamples; ++i, ++position)
    {
        // Shifted so the envelope reaches exactly zero at the end, leaving no step when the click stops.
        const double envelope = (std::exp(-position / tau) - endLevel) / (1.0 - endLevel);
        const auto sample = static_cast<float>(gain * envelope * std::sin(phasePerSample * position));
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] += sample;
    }
}

bool ClickVoice::isActive() const
{
    return position < lengthSamples;
}
