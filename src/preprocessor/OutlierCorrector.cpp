#include "OutlierCorrector.h"
#include <cmath>
#include <algorithm>

OutlierCorrector::OutlierCorrector(
    float gradientThreshold,
    int windowSize,
    int maxIterations
)
    : gradientThreshold_(gradientThreshold),
      windowSize_(windowSize),
      maxIterations_(maxIterations) {
}

OutlierCorrector::~OutlierCorrector() {
}

std::vector<FrameData> OutlierCorrector::process(const std::vector<FrameData>& frames) {
    if (frames.size() < 3) {
        // 최소 3개 프레임 필요 (이전, 현재, 다음)
        return frames;
    }

    std::vector<FrameData> result = frames;

    // Multi-pass correction (연속된 outlier 처리)
    for (int iter = 0; iter < maxIterations_; iter++) {
        bool foundOutlier = false;

        // 경계를 제외한 모든 프레임 검사
        for (size_t i = windowSize_; i < result.size() - windowSize_; i++) {
            if (isOutlier(result, i)) {
                // Outlier 보정
                float correctedValue = correctValue(result, i);
                result[i].pitchSemitones = correctedValue;
                result[i].isOutlier = true;
                foundOutlier = true;
            }
        }

        // 더 이상 outlier가 없으면 종료
        if (!foundOutlier) {
            break;
        }
    }

    return result;
}

bool OutlierCorrector::isOutlier(const std::vector<FrameData>& frames, int index) const {
    if (index < windowSize_ || index >= static_cast<int>(frames.size()) - windowSize_) {
        return false;  // 경계는 판단 불가
    }

    float maxGradient = getMaxGradient(frames, index);
    return maxGradient > gradientThreshold_;
}

float OutlierCorrector::correctValue(const std::vector<FrameData>& frames, int index) const {
    if (index < windowSize_ || index >= static_cast<int>(frames.size()) - windowSize_) {
        return frames[index].pitchSemitones;  // 경계는 보정 불가
    }

    // Weighted average 계산
    // 가까운 프레임일수록 가중치 높음
    float sumWeights = 0.0f;
    float sumValues = 0.0f;

    for (int offset = 1; offset <= windowSize_; offset++) {
        float weight = 1.0f / offset;  // 거리 반비례

        // 이전 프레임
        if (index - offset >= 0) {
            sumWeights += weight;
            sumValues += frames[index - offset].pitchSemitones * weight;
        }

        // 다음 프레임
        if (index + offset < static_cast<int>(frames.size())) {
            sumWeights += weight;
            sumValues += frames[index + offset].pitchSemitones * weight;
        }
    }

    if (sumWeights > 0.0f) {
        return sumValues / sumWeights;
    }

    return frames[index].pitchSemitones;  // Fallback
}

float OutlierCorrector::calculateGradient(float value1, float value2) const {
    return std::abs(value2 - value1);
}

float OutlierCorrector::getMaxGradient(const std::vector<FrameData>& frames, int index) const {
    float currValue = frames[index].pitchSemitones;
    float maxGradient = 0.0f;

    // windowSize 범위 내의 모든 프레임과의 gradient 계산
    for (int offset = 1; offset <= windowSize_; offset++) {
        // 이전 프레임과의 변화율
        if (index - offset >= 0) {
            float prevGradient = calculateGradient(
                frames[index - offset].pitchSemitones,
                currValue
            );
            maxGradient = std::max(maxGradient, prevGradient);
        }

        // 다음 프레임과의 변화율
        if (index + offset < static_cast<int>(frames.size())) {
            float nextGradient = calculateGradient(
                currValue,
                frames[index + offset].pitchSemitones
            );
            maxGradient = std::max(maxGradient, nextGradient);
        }
    }

    return maxGradient;
}

void OutlierCorrector::setGradientThreshold(float threshold) {
    gradientThreshold_ = threshold;
}

void OutlierCorrector::setWindowSize(int size) {
    windowSize_ = std::max(1, size);  // 최소 1
}
