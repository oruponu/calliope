#include "audio/ClickVoice.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <ranges>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int clickLength = 1440;

std::vector<float> renderMono(ClickVoice& voice, int startSample, int numSamples, int bufferSize, float gain = 1.0f)
{
    std::vector<float> buffer(static_cast<std::size_t>(bufferSize), 0.0f);
    float* channels[] = {buffer.data()};
    voice.render(channels, 1, startSample, numSamples, gain);
    return buffer;
}

bool isSilent(float sample)
{
    return sample == 0.0f;
}

int countSignChanges(const std::vector<float>& samples)
{
    int count = 0;
    for (std::size_t i = 1; i < samples.size(); ++i)
        if (samples[i - 1] * samples[i] < 0.0f)
            ++count;
    return count;
}
} // namespace

TEST_CASE("voice is silent until triggered", "[audio][click]")
{
    ClickVoice voice;
    voice.prepare(sampleRate);

    CHECK_FALSE(voice.isActive());
    CHECK(std::ranges::all_of(renderMono(voice, 0, 256, 256), isSilent));
}

TEST_CASE("triggered voice starts at the requested sample", "[audio][click]")
{
    ClickVoice voice;
    voice.prepare(sampleRate);
    voice.trigger(false);

    const auto out = renderMono(voice, 10, 54, 64);
    CHECK(std::ranges::all_of(out | std::views::take(11), isSilent));
    CHECK(out[11] != 0.0f);
    CHECK(voice.isActive());
}

TEST_CASE("voice falls silent after its length", "[audio][click]")
{
    ClickVoice voice;
    voice.prepare(sampleRate);
    voice.trigger(true);

    const auto out = renderMono(voice, 0, 2000, 2000);
    CHECK_FALSE(std::ranges::all_of(out | std::views::take(clickLength), isSilent));
    CHECK(std::ranges::all_of(out | std::views::drop(clickLength), isSilent));
    CHECK_FALSE(voice.isActive());
}

TEST_CASE("retrigger restarts the click from its head", "[audio][click]")
{
    ClickVoice fresh;
    fresh.prepare(sampleRate);
    fresh.trigger(false);
    const auto expected = renderMono(fresh, 0, 100, 100);

    ClickVoice voice;
    voice.prepare(sampleRate);
    voice.trigger(false);
    renderMono(voice, 0, 300, 300);
    voice.trigger(false);

    CHECK(renderMono(voice, 0, 100, 100) == expected);
}

TEST_CASE("accented click has a higher pitch than a plain click", "[audio][click]")
{
    ClickVoice accented;
    accented.prepare(sampleRate);
    accented.trigger(true);

    ClickVoice plain;
    plain.prepare(sampleRate);
    plain.trigger(false);

    CHECK(countSignChanges(renderMono(accented, 0, clickLength, clickLength)) >
          countSignChanges(renderMono(plain, 0, clickLength, clickLength)));
}

TEST_CASE("gain scales the output", "[audio][click]")
{
    ClickVoice full;
    full.prepare(sampleRate);
    full.trigger(false);
    const auto fullOut = renderMono(full, 0, 200, 200, 1.0f);

    ClickVoice half;
    half.prepare(sampleRate);
    half.trigger(false);
    const auto halfOut = renderMono(half, 0, 200, 200, 0.5f);

    for (std::size_t i = 0; i < fullOut.size(); ++i)
        CHECK_THAT(halfOut[i], WithinAbs(fullOut[i] * 0.5f, 1e-6));
}

TEST_CASE("preparing at a new sample rate silences the voice and rescales its length", "[audio][click]")
{
    ClickVoice voice;
    voice.prepare(sampleRate);
    voice.trigger(false);
    renderMono(voice, 0, 100, 100);

    voice.prepare(96000.0);
    CHECK_FALSE(voice.isActive());

    voice.trigger(false);
    const auto out = renderMono(voice, 0, 4000, 4000);
    CHECK_FALSE(std::ranges::all_of(out | std::views::drop(clickLength) | std::views::take(clickLength), isSilent));
    CHECK(std::ranges::all_of(out | std::views::drop(2 * clickLength), isSilent));
}
