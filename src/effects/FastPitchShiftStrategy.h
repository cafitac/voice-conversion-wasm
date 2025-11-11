#ifndef FASTPITCHSHIFTSTRATEGY_H
#define FASTPITCHSHIFTSTRATEGY_H

#include "IPitchShiftStrategy.h"
#include "PitchShifter.h"
#include <memory>

/**
 * FastPitchShiftStrategy
 *
 * 빠른 pitch shifting 전략 (Simple Resampling)
 * 기존 PitchShifter를 래핑
 *
 * 장점:
 * - 매우 빠른 처리 속도 (< 1 ms)
 * - 단순한 구현
 *
 * 단점:
 * - Aliasing 발생
 * - Formant 왜곡 (Chipmunk effect)
 * - 음질 저하
 *
 * 사용 권장:
 * - 실시간 처리가 중요한 경우
 * - 음질보다 속도가 우선인 경우
 */
class FastPitchShiftStrategy : public IPitchShiftStrategy {
public:
    FastPitchShiftStrategy();
    ~FastPitchShiftStrategy() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;
    const char* getName() const override { return "FastPitchShift (Simple Resampling)"; }

private:
    std::unique_ptr<PitchShifter> shifter_;
};

#endif // FASTPITCHSHIFTSTRATEGY_H
