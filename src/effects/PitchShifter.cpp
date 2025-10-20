#include "PitchShifter.h"
#include <cmath>
#include <algorithm>

PitchShifter::PitchShifter() {
}

PitchShifter::~PitchShifter() {
}

AudioBuffer PitchShifter::shiftPitch(const AudioBuffer& input, float semitones) {
    // 반음을 비율로 변환 (12 semitones = 2x frequency)
    float ratio = std::pow(2.0f, semitones / 12.0f);
    return shiftPitchByRatio(input, ratio);
}

AudioBuffer PitchShifter::shiftPitchByRatio(const AudioBuffer& input, float ratio) {
    if (ratio <= 0.0f) ratio = 1.0f;

    AudioBuffer output(input.getSampleRate(), input.getChannels());
    const auto& inputData = input.getData();

    // 리샘플링
    auto resampled = resample(inputData, ratio);
    output.setData(resampled);

    return output;
}

AudioBuffer PitchShifter::shiftPitchCurve(const AudioBuffer& input, const std::vector<float>& pitchCurve) {
    // 시간에 따라 변하는 pitch shift 적용
    // 단순화를 위해 프레임별로 처리
    AudioBuffer output(input.getSampleRate(), input.getChannels());
    const auto& inputData = input.getData();

    if (pitchCurve.empty()) {
        output.setData(inputData);
        return output;
    }

    int sampleRate = input.getSampleRate();
    int frameSize = static_cast<int>(0.02f * sampleRate); // 20ms frames
    std::vector<float> outputData;

    for (size_t i = 0; i < inputData.size(); i += frameSize) {
        size_t end = std::min(i + frameSize, inputData.size());
        std::vector<float> frame(inputData.begin() + i, inputData.begin() + end);

        // 현재 프레임에 대한 pitch shift 비율 계산
        size_t curveIndex = (i * pitchCurve.size()) / inputData.size();
        curveIndex = std::min(curveIndex, pitchCurve.size() - 1);
        float ratio = std::pow(2.0f, pitchCurve[curveIndex] / 12.0f);

        // 프레임에 pitch shift 적용
        auto shifted = resample(frame, ratio);
        outputData.insert(outputData.end(), shifted.begin(), shifted.end());
    }

    output.setData(outputData);
    return output;
}

std::vector<float> PitchShifter::resample(const std::vector<float>& input, float ratio) {
    if (input.empty() || ratio <= 0.0f) return input;

    size_t outputSize = static_cast<size_t>(input.size() / ratio);
    std::vector<float> output(outputSize);

    for (size_t i = 0; i < outputSize; ++i) {
        float position = i * ratio;
        output[i] = interpolate(input, position);
    }

    return output;
}

float PitchShifter::interpolate(const std::vector<float>& data, float position) {
    if (data.empty()) return 0.0f;

    int index = static_cast<int>(position);
    float fraction = position - index;

    if (index < 0) return data[0];
    if (index >= static_cast<int>(data.size()) - 1) return data.back();

    // Linear interpolation
    return data[index] * (1.0f - fraction) + data[index + 1] * fraction;
}
