#ifndef IPITCHSHIFTSTRATEGY_H
#define IPITCHSHIFTSTRATEGY_H

#include "../audio/AudioBuffer.h"

/**
 * IPitchShiftStrategy
 *
 * Pitch shifting 알고리즘을 위한 Strategy Pattern 인터페이스
 * 런타임에 알고리즘을 교체 가능하도록 설계
 *
 * 사용 예:
 *   - FastPitchShiftStrategy: 빠른 처리 (낮은 품질)
 *   - HighQualityPitchShiftStrategy: 고품질 처리 (Phase Vocoder)
 */
class IPitchShiftStrategy {
public:
    virtual ~IPitchShiftStrategy() = default;

    /**
     * Pitch shift 적용
     *
     * @param input 입력 오디오 버퍼
     * @param semitones Pitch shift 양 (semitones 단위)
     * @return Pitch shifted 오디오 버퍼
     */
    virtual AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) = 0;

    /**
     * 전략 이름 반환 (디버깅/로깅용)
     */
    virtual const char* getName() const = 0;
};

#endif // IPITCHSHIFTSTRATEGY_H
