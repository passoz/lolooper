/*
 * Copyright (c) 2025 Fernando Passos. All rights reserved.
 * Licensed under the MIT License.
 *
 * Lolooper — Live performance looper for Brazilian music
 */

#include "Mixer.h"
#include "SampleLib.h"
#include <juce_dsp/juce_dsp.h>

//==============================================================================
void Mixer::renderStep(int step,
                       const float pattern[14][16],
                       const float volumes[14],
                       const float pans[14],
                       const bool mutes[14],
                       const bool solos[14],
                       const float accents[14],
                       juce::AudioBuffer<float>& output,
                       double sampleRate)
{
    // Determine if any solo is active
    bool anySolo = false;
    for (int t = 0; t < 14; ++t)
    {
        if (solos[t])
        {
            anySolo = true;
            break;
        }
    }

    // For each track, get sample and write to output
    for (int track = 0; track < 14; ++track)
    {
        // Skip if muted (or not soloed when another track is soloed)
        if (mutes[track])
            continue;

        if (anySolo && !solos[track])
            continue;

        // Get the velocity for this step
        float velocity = pattern[track][step];
        if (velocity <= 0.0f)
            continue;

        // Apply accent multiplier
        velocity *= accents[track];
        if (velocity <= 0.0f)
            continue;

        // Get the sample buffer
        const auto* sampleBuffer = m_sampleLib->getSample(
            juce::String(track), "default");

        if (sampleBuffer == nullptr)
            continue;

        // Get track parameters
        float volume = volumes[track];
        float pan = pans[track];

        // Calculate gain per channel
        // pan: 0 = full left, 0.5 = center, 1.0 = full right
        float leftGain = volume * velocity * std::sqrt(1.0f - pan);
        float rightGain = volume * velocity * std::sqrt(pan);

        int numOutputChannels = output.getNumChannels();
        int numOutputSamples = output.getNumSamples();

        int numSampleChannels = sampleBuffer->getNumChannels();
        int numSampleSamples = sampleBuffer->getNumSamples();

        // Write sample data into the output buffer
        for (int s = 0; s < numOutputSamples; ++s)
        {
            // Calculate read position (wrapped or stopped)
            int readPos = (int)(m_trackReadPos[track] + s);

            if (readPos >= numSampleSamples)
                break; // Sample ended

            float sampleVal;

            if (numSampleChannels == 1)
            {
                // Mono sample: same value for both channels
                sampleVal = sampleBuffer->getSample(0, readPos);
            }
            else
            {
                // Stereo sample
                float leftSample = sampleBuffer->getSample(0, readPos);
                float rightSample = sampleBuffer->getSample(1, readPos);

                // Mix into output — for stereo samples, apply pan after summing
                output.addSample(0, s, leftSample * leftGain);
                output.addSample(1, s, rightSample * rightGain);
                continue;
            }

            // Mono sample: apply pan
            output.addSample(0, s, sampleVal * leftGain);
            output.addSample(1, s, sampleVal * rightGain);
        }
    }

    // Update read positions (they reset each step for percussive samples)
    for (int t = 0; t < 14; ++t)
        m_trackReadPos[t] = 0;
}

//==============================================================================
void Mixer::applySoftClip(juce::AudioBuffer<float>& buffer)
{
    // Soft clipping to prevent harsh distortion
    // Uses a simple tanh-like curve
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = data[i];

            // Soft clip: cubic curve for gentle limiting
            if (sample > 1.0f)
                sample = 1.0f;
            else if (sample < -1.0f)
                sample = -1.0f;
            else
                sample = sample - (sample * sample * sample) / 3.0f;

            data[i] = sample;
        }
    }
}
