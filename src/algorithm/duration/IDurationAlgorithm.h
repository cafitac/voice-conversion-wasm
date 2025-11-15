#ifndef IDURATIONALGORITHM_H
#define IDURATIONALGORITHM_H

#include "../../audio/AudioBuffer.h"

/**
 * IDurationAlgorithm
 *
 * Time stretching (duration modification) 알고리즘 인터페이스
 *
 * 모든 time stretch 알고리즘이 구현해야 하는 공통 인터페이스입니다.
 * Strategy 패턴을 사용하여 알고리즘을 교체 가능하게 합니다.
 *
 * 알고리즘 종류:
 * - WSOLA: Waveform Similarity Overlap-Add, 빠름
 * - Phase Vocoder: Frequency-domain, 고품질
 * - SoundTouch: 검증된 라이브러리, 안정적
 * - RubberBand: 최고 품질, GPL 라이선스
 */
class IDurationAlgorithm {
public:
    virtual ~IDurationAlgorithm() = default;

    /**
     * Time stretch 적용 (pitch 유지)
     *
     * @param input 입력 오디오
     * @param ratio 재생 속도 비율 (0.5 = 2배 느리게, 2.0 = 2배 빠르게)
     * @return 처리된 오디오
     */
    virtual AudioBuffer stretch(const AudioBuffer& input, float ratio) = 0;

    /**
     * 알고리즘 이름 반환
     */
    virtual const char* getName() const = 0;

    /**
     * 알고리즘 설명 반환
     */
    virtual const char* getDescription() const = 0;

    /**
     * Real-time 처리 가능 여부
     */
    virtual bool supportsRealtime() const { return false; }

    /**
     * 지원하는 최소/최대 ratio
     */
    virtual float getMinRatio() const { return 0.5f; }
    virtual float getMaxRatio() const { return 2.0f; }
};

#endif // IDURATIONALGORITHM_H
