#include "FastTimeStretchStrategy.h"
#include <vector>
#include <cmath>

FastTimeStretchStrategy::FastTimeStretchStrategy() {
}

FastTimeStretchStrategy::~FastTimeStretchStrategy() {
}

AudioBuffer FastTimeStretchStrategy::stretch(const AudioBuffer& input, float ratio) {
    if (ratio <= 0.0f) ratio = 1.0f;

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();

    auto stretched = simpleFrameStretch(inputData, ratio, sampleRate, channels);

    AudioBuffer result(sampleRate, channels);
    result.setData(stretched);

    return result;
}

std::vector<float> FastTimeStretchStrategy::simpleFrameStretch(
    const std::vector<float>& input,
    float ratio,
    int sampleRate,
    int channels) {

    if (input.empty()) return input;

    // Frame 크기: 10ms (짧은 프레임으로 클릭 노이즈 최소화)
    int frameSize = static_cast<int>(0.01f * sampleRate) * channels;
    if (frameSize <= 0) frameSize = channels;

    size_t outputSize = static_cast<size_t>(input.size() * ratio);
    std::vector<float> output;
    output.reserve(outputSize);

    if (ratio >= 1.0f) {
        // 느리게: frame 복제
        int inputPos = 0;
        float accum = 0.0f;

        while (inputPos < static_cast<int>(input.size())) {
            int remaining = static_cast<int>(input.size()) - inputPos;
            int currentFrameSize = std::min(frameSize, remaining);

            // 현재 frame 복사
            for (int i = 0; i < currentFrameSize; ++i) {
                output.push_back(input[inputPos + i]);
            }

            accum += ratio;

            // ratio만큼 복제
            while (accum >= 2.0f && output.size() < outputSize) {
                for (int i = 0; i < currentFrameSize; ++i) {
                    output.push_back(input[inputPos + i]);
                }
                accum -= 1.0f;
            }

            inputPos += currentFrameSize;
        }
    } else {
        // 빠르게: frame 건너뛰기
        float skipRatio = 1.0f / ratio;
        float inputPos = 0.0f;

        while (static_cast<int>(inputPos) < static_cast<int>(input.size())) {
            int pos = static_cast<int>(inputPos);
            int remaining = static_cast<int>(input.size()) - pos;
            int currentFrameSize = std::min(frameSize, remaining);

            // 현재 frame 복사
            for (int i = 0; i < currentFrameSize && pos + i < static_cast<int>(input.size()); ++i) {
                output.push_back(input[pos + i]);
            }

            inputPos += currentFrameSize * skipRatio;
        }
    }

    // 출력 크기 조정
    if (output.size() > outputSize) {
        output.resize(outputSize);
    } else if (output.size() < outputSize) {
        // 부족한 경우 마지막 샘플로 채우기
        float lastSample = output.empty() ? 0.0f : output.back();
        while (output.size() < outputSize) {
            output.push_back(lastSample);
        }
    }

    return output;
}
