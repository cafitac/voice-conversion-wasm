#include "FramePitchModifier.h"
#include "HighQualityPitchShiftStrategy.h"
#include "../audio/AudioBuffer.h"
#include <cmath>

FramePitchModifier::FramePitchModifier(IPitchShiftStrategy* strategy)
    : strategy_(strategy), ownsStrategy_(false) {
    // 전략이 주어지지 않으면 기본값으로 HighQuality 사용
    if (!strategy_) {
        strategy_ = new HighQualityPitchShiftStrategy(1024, 256);
        ownsStrategy_ = true;
    }
}

FramePitchModifier::~FramePitchModifier() {
    if (ownsStrategy_ && strategy_) {
        delete strategy_;
    }
}

void FramePitchModifier::setStrategy(IPitchShiftStrategy* strategy) {
    if (ownsStrategy_ && strategy_) {
        delete strategy_;
    }
    strategy_ = strategy;
    ownsStrategy_ = false;
}

void FramePitchModifier::applyPitchShifts(
    std::vector<FrameData>& frames,
    const std::vector<float>& pitchShifts,
    int sampleRate
) {
    if (frames.empty() || pitchShifts.empty() || !strategy_) {
        return;
    }

    for (size_t i = 0; i < frames.size(); ++i) {
        // pitchShifts 배열 크기보다 많은 프레임이 있으면 마지막 값 재사용
        float semitones = (i < pitchShifts.size()) ? pitchShifts[i] : pitchShifts.back();

        // semitones가 0이면 수정 안함
        if (std::abs(semitones) < 0.01f) {
            continue;
        }

        // 프레임을 AudioBuffer로 변환
        AudioBuffer frameBuffer(sampleRate, 1);
        frameBuffer.setData(frames[i].samples);

        // Pitch shift 적용 (Strategy Pattern 사용)
        AudioBuffer shifted = strategy_->shiftPitch(frameBuffer, semitones);

        // 결과를 다시 프레임에 저장
        frames[i].samples = shifted.getData();

        // RMS 재계산
        frames[i].rms = calculateRMS(frames[i].samples);
    }
}

float FramePitchModifier::calculateRMS(const std::vector<float>& samples) {
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
