#include "TimeScaleModifier.h"

TimeScaleModifier::TimeScaleModifier() {
}

TimeScaleModifier::~TimeScaleModifier() {
}

std::vector<float> TimeScaleModifier::createUniformTimeRatios(size_t numFrames, float ratio) {
    return std::vector<float>(numFrames, ratio);
}

float TimeScaleModifier::calculateExpectedDuration(float originalDuration, float ratio) {
    return originalDuration * ratio;
}

std::vector<float> TimeScaleModifier::createVariableTimeRatios(
    size_t numFrames,
    const std::vector<float>& ratios
) {
    std::vector<float> result(numFrames);

    for (size_t i = 0; i < numFrames; ++i) {
        // ratios 배열 크기보다 많은 프레임이 있으면 마지막 값 재사용
        result[i] = (i < ratios.size()) ? ratios[i] : ratios.back();
    }

    return result;
}
