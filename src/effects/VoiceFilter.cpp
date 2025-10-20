#include "VoiceFilter.h"
#include <cmath>
#include <algorithm>

VoiceFilter::VoiceFilter() {
}

VoiceFilter::~VoiceFilter() {
}

AudioBuffer VoiceFilter::applyFilter(const AudioBuffer& input, FilterType type, float param1, float param2) {
    switch (type) {
        case FilterType::LOW_PASS:
            return applyLowPass(input, param1 * 5000.0f + 500.0f);
        case FilterType::HIGH_PASS:
            return applyHighPass(input, param1 * 3000.0f + 100.0f);
        case FilterType::BAND_PASS:
            return applyBandPass(input, param1 * 2000.0f + 200.0f, param2 * 3000.0f + 1000.0f);
        case FilterType::ROBOT:
            return applyRobot(input);
        case FilterType::ECHO:
            return applyEcho(input, param1 * 0.5f + 0.1f, param2 * 0.7f + 0.1f);
        case FilterType::REVERB:
            return applyReverb(input, param1, param2);
        default:
            return input;
    }
}

AudioBuffer VoiceFilter::applyLowPass(const AudioBuffer& input, float cutoff) {
    AudioBuffer output = input;
    auto& data = output.getData();
    applySimpleLowPass(data, cutoff, input.getSampleRate());
    return output;
}

AudioBuffer VoiceFilter::applyHighPass(const AudioBuffer& input, float cutoff) {
    AudioBuffer output = input;
    auto& data = output.getData();
    applySimpleHighPass(data, cutoff, input.getSampleRate());
    return output;
}

AudioBuffer VoiceFilter::applyBandPass(const AudioBuffer& input, float lowCutoff, float highCutoff) {
    AudioBuffer output = applyHighPass(input, lowCutoff);
    return applyLowPass(output, highCutoff);
}

AudioBuffer VoiceFilter::applyRobot(const AudioBuffer& input) {
    // 간단한 로봇 효과: 사인파 모듈레이션
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();

    float modFreq = 30.0f; // Hz
    for (size_t i = 0; i < data.size(); ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float modulator = std::sin(2.0f * M_PI * modFreq * t);
        data[i] *= (0.5f + 0.5f * modulator);
    }

    return output;
}

AudioBuffer VoiceFilter::applyEcho(const AudioBuffer& input, float delay, float feedback) {
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();
    int delaySamples = static_cast<int>(delay * sampleRate);

    if (delaySamples >= static_cast<int>(data.size())) {
        return output;
    }

    for (int i = delaySamples; i < static_cast<int>(data.size()); ++i) {
        data[i] += data[i - delaySamples] * feedback;
        // 클리핑 방지
        data[i] = std::max(-1.0f, std::min(1.0f, data[i]));
    }

    return output;
}

AudioBuffer VoiceFilter::applyReverb(const AudioBuffer& input, float roomSize, float damping) {
    // 간단한 리버브: 여러 딜레이의 조합
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();

    // 여러 딜레이 라인
    std::vector<int> delays = {
        static_cast<int>(0.029f * roomSize * sampleRate),
        static_cast<int>(0.037f * roomSize * sampleRate),
        static_cast<int>(0.041f * roomSize * sampleRate),
        static_cast<int>(0.043f * roomSize * sampleRate)
    };

    float feedbackGain = 0.3f * (1.0f - damping);

    for (int delay : delays) {
        if (delay >= static_cast<int>(data.size())) continue;

        for (int i = delay; i < static_cast<int>(data.size()); ++i) {
            data[i] += data[i - delay] * feedbackGain;
            data[i] = std::max(-1.0f, std::min(1.0f, data[i]));
        }
    }

    return output;
}

void VoiceFilter::applySimpleLowPass(std::vector<float>& data, float cutoff, int sampleRate) {
    if (data.size() < 2) return;

    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (rc + dt);

    for (size_t i = 1; i < data.size(); ++i) {
        data[i] = data[i - 1] + alpha * (data[i] - data[i - 1]);
    }
}

void VoiceFilter::applySimpleHighPass(std::vector<float>& data, float cutoff, int sampleRate) {
    if (data.size() < 2) return;

    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = rc / (rc + dt);

    std::vector<float> original = data;
    for (size_t i = 1; i < data.size(); ++i) {
        data[i] = alpha * (data[i - 1] + original[i] - original[i - 1]);
    }
}
