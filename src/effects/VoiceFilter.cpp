#include "VoiceFilter.h"
#include <cmath>
#include <algorithm>

VoiceFilter::VoiceFilter() {
}

VoiceFilter::~VoiceFilter() {
}

AudioBuffer VoiceFilter::applyFilter(const AudioBuffer& input, FilterType type, float param1, float param2) {
    // 원본 RMS 계산 (볼륨 보정용)
    float originalRMS = calculateRMS(input.getData());
    
    AudioBuffer result;
    switch (type) {
        case FilterType::LOW_PASS: {
            // 🐻 곰: 아주 낮은 저음 위주 (굵고 둔한 느낌)
            // param1: 0.0 ~ 1.0 -> 약 120Hz ~ 400Hz
            float minCut = 120.0f;
            float maxCut = 400.0f;
            float cutoff = minCut + (maxCut - minCut) * std::clamp(param1, 0.0f, 1.0f);
            result = applyLowPass(input, cutoff);
            break;
        }
        case FilterType::HIGH_PASS: {
            // 🐰 토끼: 아주 높은 고음 위주 (얇고 날카로운 느낌)
            // param1: 0.0 ~ 1.0 -> 약 2500Hz ~ 6000Hz
            float minCut = 2500.0f;
            float maxCut = 6000.0f;
            float cutoff = minCut + (maxCut - minCut) * std::clamp(param1, 0.0f, 1.0f);
            result = applyHighPass(input, cutoff);
            break;
        }
        case FilterType::BAND_PASS: {
            // 📻 라디오: 사람 목소리 대역만 남기는 느낌 (전화기/라디오 톤)
            // lowCutoff:  ~300Hz, highCutoff: ~3kHz 근처를 기본값으로 두고 param으로 미세 조정
            float baseLow = 300.0f;
            float baseHigh = 3000.0f;
            // param1: 저역 쪽 보정 (0.5를 기준으로 ±150Hz)
            float lowOffset = (std::clamp(param1, 0.0f, 1.0f) - 0.5f) * 300.0f;
            // param2: 고역 쪽 보정 (0.5를 기준으로 ±800Hz)
            float highOffset = (std::clamp(param2, 0.0f, 1.0f) - 0.5f) * 1600.0f;
            float lowCutoff = std::max(80.0f, baseLow + lowOffset);
            float highCutoff = std::min(6000.0f, baseHigh + highOffset);
            if (highCutoff <= lowCutoff + 100.0f) {
                highCutoff = lowCutoff + 100.0f;
            }
            result = applyBandPass(input, lowCutoff, highCutoff);
            break;
        }
        case FilterType::ROBOT:
            result = applyRobot(input);
            break;
        case FilterType::ECHO:
            result = applyEcho(input, param1 * 0.5f + 0.1f, param2 * 0.7f + 0.1f);
            break;
        case FilterType::REVERB:
            result = applyReverb(input, param1, param2);
            break;
        default:
            return input;
    }
    
    // 필터 적용 후 RMS 계산
    float filteredRMS = calculateRMS(result.getData());
    
    // 볼륨 보정: 원본 RMS에 맞춰 조정 (단, 클리핑 방지)
    if (filteredRMS > 0.0001f && originalRMS > 0.0001f) {
        float gain = originalRMS / filteredRMS;
        // 과도한 증폭 방지 (최대 3배)
        gain = std::min(gain, 3.0f);
        
        auto& data = result.getData();
        for (float& sample : data) {
            sample *= gain;
            // 클리핑 방지
            sample = std::max(-1.0f, std::min(1.0f, sample));
        }
    }
    
    return result;
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

float VoiceFilter::calculateRMS(const std::vector<float>& data) {
    if (data.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (float sample : data) {
        sum += sample * sample;
    }
    return std::sqrt(sum / data.size());
}
