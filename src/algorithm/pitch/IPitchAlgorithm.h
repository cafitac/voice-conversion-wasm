#ifndef IPITCHALGORITHM_H
#define IPITCHALGORITHM_H

#include "../../audio/AudioBuffer.h"

/**
 * IPitchAlgorithm
 *
 * Pitch shifting 알고리즘 인터페이스
 *
 * 모든 pitch shift 알고리즘이 구현해야 하는 공통 인터페이스입니다.
 * Strategy 패턴을 사용하여 알고리즘을 교체 가능하게 합니다.
 *
 * 알고리즘 종류:
 * - PSOLA: Time-domain, 빠름, 음성 최적화
 * - Phase Vocoder: Frequency-domain, 고품질, 범용
 * - SoundTouch: 검증된 라이브러리, 안정적
 * - RubberBand: 최고 품질, GPL 라이선스
 */
class IPitchAlgorithm {
public:
    virtual ~IPitchAlgorithm() = default;

    /**
     * Pitch shift 적용
     *
     * @param input 입력 오디오
     * @param semitones 반음 단위 pitch shift (-12 ~ +12)
     * @return 처리된 오디오
     */
    virtual AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) = 0;

    /**
     * 알고리즘 이름 반환
     */
    virtual const char* getName() const = 0;

    /**
     * 알고리즘 설명 반환
     */
    virtual const char* getDescription() const = 0;

    /**
     * Formant preservation 지원 여부
     */
    virtual bool supportsFormantPreservation() const { return false; }

    /**
     * Real-time 처리 가능 여부
     */
    virtual bool supportsRealtime() const { return false; }
};

#endif // IPITCHALGORITHM_H
