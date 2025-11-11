#include "FrameReconstructor.h"
#include <cmath>

FrameReconstructor::FrameReconstructor() {
}

FrameReconstructor::~FrameReconstructor() {
}

std::vector<float> FrameReconstructor::createHanningWindow(int size) {
    std::vector<float> window(size);
    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }
    return window;
}

AudioBuffer FrameReconstructor::reconstruct(
    const std::vector<FrameData>& frames,
    int sampleRate,
    int channels,
    float baseHopSize,
    const std::vector<float>& timeRatios
) {
    if (frames.empty()) {
        return AudioBuffer(sampleRate, channels);
    }

    // 프레임 크기
    int frameSamples = frames[0].samples.size();

    // Hanning window 생성
    std::vector<float> window = createHanningWindow(frameSamples);

    // 출력 버퍼 크기 계산 (timeRatios 고려)
    float totalTime = 0.0f;
    for (size_t i = 0; i < frames.size(); ++i) {
        float ratio = (i < timeRatios.size()) ? timeRatios[i] : 1.0f;
        totalTime += baseHopSize * ratio;
    }
    totalTime += (frameSamples / static_cast<float>(sampleRate * channels)); // 마지막 프레임 길이

    int totalSamples = static_cast<int>(totalTime * sampleRate * channels) + frameSamples;
    std::vector<float> output(totalSamples, 0.0f);
    std::vector<float> windowSum(totalSamples, 0.0f);

    // Overlap-Add 합성 (프레임별로 다른 hop size 사용)
    float currentTime = 0.0f;
    for (size_t i = 0; i < frames.size(); ++i) {
        int position = static_cast<int>(currentTime * sampleRate * channels);

        // 프레임 배치
        for (int j = 0; j < frameSamples && position + j < totalSamples; ++j) {
            float windowedSample = frames[i].samples[j] * window[j];
            output[position + j] += windowedSample;
            windowSum[position + j] += window[j];
        }

        // 다음 프레임 위치 계산 (timeRatio 적용)
        float ratio = (i < timeRatios.size()) ? timeRatios[i] : 1.0f;
        currentTime += baseHopSize * ratio;
    }

    // 정규화 (window sum으로 나누기)
    for (size_t i = 0; i < output.size(); ++i) {
        if (windowSum[i] > 0.0f) {
            output[i] /= windowSum[i];
        }
    }

    // AudioBuffer 생성 및 반환
    AudioBuffer result(sampleRate, channels);
    result.setData(output);

    return result;
}
