#include "AudioProcessor.h"
#include <algorithm>
#include <cmath>

AudioProcessor::AudioProcessor() {
}

AudioProcessor::~AudioProcessor() {
}

void AudioProcessor::normalize(AudioBuffer& buffer) {
    auto& data = buffer.getData();
    if (data.empty()) return;

    // 최대 절대값 찾기
    float maxVal = 0.0f;
    for (float sample : data) {
        maxVal = std::max(maxVal, std::abs(sample));
    }

    // 정규화
    if (maxVal > 0.0f) {
        float scale = 1.0f / maxVal;
        for (float& sample : data) {
            sample *= scale;
        }
    }
}

void AudioProcessor::amplify(AudioBuffer& buffer, float gain) {
    auto& data = buffer.getData();
    for (float& sample : data) {
        sample *= gain;
        // 클리핑 방지
        sample = std::max(-1.0f, std::min(1.0f, sample));
    }
}

void AudioProcessor::applyLowPassFilter(AudioBuffer& buffer, float cutoffFreq) {
    // 간단한 1차 저역 통과 필터 (RC 필터)
    auto& data = buffer.getData();
    if (data.size() < 2) return;

    float sampleRate = static_cast<float>(buffer.getSampleRate());
    float rc = 1.0f / (2.0f * M_PI * cutoffFreq);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (rc + dt);

    for (size_t i = 1; i < data.size(); ++i) {
        data[i] = data[i - 1] + alpha * (data[i] - data[i - 1]);
    }
}

void AudioProcessor::trimSilence(AudioBuffer& buffer, float threshold) {
    auto& data = buffer.getData();
    if (data.empty()) return;

    // 시작 부분의 무음 찾기
    size_t start = 0;
    while (start < data.size() && std::abs(data[start]) < threshold) {
        ++start;
    }

    // 끝 부분의 무음 찾기
    size_t end = data.size();
    while (end > start && std::abs(data[end - 1]) < threshold) {
        --end;
    }

    // 트림
    if (start > 0 || end < data.size()) {
        std::vector<float> trimmed(data.begin() + start, data.begin() + end);
        data = std::move(trimmed);
    }
}

float AudioProcessor::calculateRMS(const std::vector<float>& data, size_t start, size_t length) {
    if (start + length > data.size()) return 0.0f;

    float sum = 0.0f;
    for (size_t i = start; i < start + length; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / length);
}
