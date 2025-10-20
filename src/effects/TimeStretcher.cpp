#include "TimeStretcher.h"
#include <cmath>
#include <algorithm>

TimeStretcher::TimeStretcher() {
}

TimeStretcher::~TimeStretcher() {
}

AudioBuffer TimeStretcher::stretch(const AudioBuffer& input, float ratio) {
    if (ratio <= 0.0f) ratio = 1.0f;

    AudioBuffer output(input.getSampleRate(), input.getChannels());
    const auto& inputData = input.getData();

    auto stretched = wsolaStretch(inputData, ratio, input.getSampleRate());
    output.setData(stretched);

    return output;
}

AudioBuffer TimeStretcher::stretchCurve(const AudioBuffer& input, const std::vector<float>& stretchCurve) {
    // 시간에 따라 변하는 time stretch 적용
    AudioBuffer output(input.getSampleRate(), input.getChannels());
    const auto& inputData = input.getData();

    if (stretchCurve.empty()) {
        output.setData(inputData);
        return output;
    }

    // 간단한 구현: 평균 stretch ratio 사용
    float avgRatio = 0.0f;
    for (float val : stretchCurve) {
        avgRatio += val;
    }
    avgRatio /= stretchCurve.size();

    return stretch(input, avgRatio);
}

std::vector<float> TimeStretcher::wsolaStretch(const std::vector<float>& input, float ratio, int sampleRate) {
    if (input.empty()) return input;

    int frameSize = static_cast<int>(0.02f * sampleRate); // 20ms
    int hopSizeInput = frameSize / 2;
    int hopSizeOutput = static_cast<int>(hopSizeInput * ratio);
    int overlapSize = frameSize - hopSizeInput;

    size_t outputSize = static_cast<size_t>(input.size() * ratio);
    std::vector<float> output(outputSize, 0.0f);

    int inputPos = 0;
    int outputPos = 0;

    while (inputPos + frameSize < static_cast<int>(input.size()) && outputPos + frameSize < static_cast<int>(output.size())) {
        // 현재 프레임 추출
        std::vector<float> frame(input.begin() + inputPos, input.begin() + inputPos + frameSize);

        // 출력에 크로스페이드로 추가
        crossfade(output, frame, outputPos, overlapSize);

        inputPos += hopSizeInput;
        outputPos += hopSizeOutput;
    }

    return output;
}

int TimeStretcher::findBestMatch(const std::vector<float>& target, const std::vector<float>& search, int searchRange) {
    int bestOffset = 0;
    float minError = 1e10f;

    for (int offset = 0; offset < searchRange && offset < static_cast<int>(search.size()) - static_cast<int>(target.size()); ++offset) {
        float error = 0.0f;
        for (size_t i = 0; i < target.size(); ++i) {
            float diff = target[i] - search[offset + i];
            error += diff * diff;
        }

        if (error < minError) {
            minError = error;
            bestOffset = offset;
        }
    }

    return bestOffset;
}

void TimeStretcher::crossfade(std::vector<float>& output, const std::vector<float>& frame, int position, int overlapSize) {
    for (size_t i = 0; i < frame.size() && position + i < output.size(); ++i) {
        if (i < static_cast<size_t>(overlapSize)) {
            // 크로스페이드 영역
            float alpha = static_cast<float>(i) / overlapSize;
            output[position + i] = output[position + i] * (1.0f - alpha) + frame[i] * alpha;
        } else {
            // 일반 복사
            output[position + i] = frame[i];
        }
    }
}
