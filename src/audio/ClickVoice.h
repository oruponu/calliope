#pragma once

class ClickVoice
{
public:
    static constexpr double lengthSeconds = 0.03;
    static constexpr double accentFrequency = 1500.0;
    static constexpr double plainFrequency = 1000.0;

    ClickVoice();

    void prepare(double sampleRate);
    void trigger(bool accent);
    // Adds the click to channels[ch][startSample, startSample + numSamples).
    void render(float* const* channels, int numChannels, int startSample, int numSamples, float gain);
    bool isActive() const;

private:
    double sampleRate = 0.0;
    int lengthSamples = 0;
    int position = 0;
    double frequency = plainFrequency;
};
