#include "AudioPreprocessor.h"
#include <cmath>
#include <algorithm>
#include <cstring>

AudioPreprocessor::AudioPreprocessor()
    : vadThreshold_(0.02f), noiseGateEnabled_(true) {
}

AudioPreprocessor::~AudioPreprocessor() {
}

std::vector<FrameData> AudioPreprocessor::process(
    const AudioBuffer& buffer,
    float frameSize,
    float hopSize,
    float vadThreshold
) {
    std::vector<FrameData> frames;

    const auto& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int channels = buffer.getChannels();

    if (data.empty() || sampleRate <= 0) {
        return frames;
    }

    // 프레임 및 홉 크기 계산 (샘플 단위)
    int frameSamples = static_cast<int>(frameSize * sampleRate * channels);
    int hopSamples = static_cast<int>(hopSize * sampleRate * channels);

    // 최소 크기 보장
    if (frameSamples == 0) frameSamples = channels;
    if (hopSamples == 0) hopSamples = channels;

    // 프레임별로 순회
    for (size_t i = 0; i + frameSamples <= data.size(); i += hopSamples) {
        FrameData frame;

        // 시간 계산
        frame.time = static_cast<float>(i) / (sampleRate * channels);

        // 샘플 추출
        frame.samples.assign(
            data.begin() + i,
            data.begin() + i + frameSamples
        );

        // RMS 계산
        frame.rms = calculateRMS(frame.samples);

        // VAD 판단
        frame.isVoice = detectVoice(frame.rms, vadThreshold);

        frames.push_back(frame);
    }

    return frames;
}

void AudioPreprocessor::setVADThreshold(float threshold) {
    vadThreshold_ = threshold;
}

void AudioPreprocessor::setNoiseGateEnabled(bool enabled) {
    noiseGateEnabled_ = enabled;
}

float AudioPreprocessor::calculateRMS(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }

    double sumSquares = 0.0;
    for (float sample : samples) {
        sumSquares += sample * sample;
    }

    double meanSquare = sumSquares / samples.size();
    return static_cast<float>(std::sqrt(meanSquare));
}

bool AudioPreprocessor::detectVoice(float rms, float threshold) {
    return rms >= threshold;
}

