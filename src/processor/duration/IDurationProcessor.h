#ifndef IDURATIONPROCESSOR_H
#define IDURATIONPROCESSOR_H

#include "../../audio/AudioPreprocessor.h"
#include "../../audio/AudioBuffer.h"
#include <vector>

/**
 * IDurationProcessor
 *
 * Duration 처리 프로세서 인터페이스
 *
 * 모든 duration (time stretch) 알고리즘은 이 인터페이스를 구현합니다:
 * - WSOLADurationProcessor (네이티브 variable duration)
 * - PhaseVocoderDurationProcessor (네이티브 variable duration)
 * - SoundTouchDurationProcessor (frame-by-frame wrapper)
 * - RubberBandDurationProcessor (frame-by-frame wrapper)
 *
 * 동작 방식:
 * 1. FrameData를 받음 (각 프레임에 durationRatio 포함)
 * 2. 알고리즘에 따라 처리
 *    - Variable duration 지원: 각 프레임의 durationRatio 사용
 *    - Fixed duration only: frame-by-frame으로 분할하여 처리
 * 3. 처리된 FrameData 반환
 *
 * Pipeline에서 사용:
 *   PitchProcessor → DurationProcessor → Reconstructor → AudioBuffer
 */
class IDurationProcessor {
public:
    virtual ~IDurationProcessor() = default;

    /**
     * Duration 처리
     *
     * @param frames 입력 프레임들 (각 프레임에 durationRatio 포함)
     * @param sampleRate 샘플 레이트 (Hz)
     * @return 처리된 프레임들
     */
    virtual std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) = 0;

    /**
     * Variable duration 지원 여부
     *
     * @return true if 네이티브 variable duration 지원
     *         false if frame-by-frame으로 구현
     */
    virtual bool supportsVariableDuration() const = 0;

    /**
     * 프로세서 이름 반환 (디버깅/로깅용)
     */
    virtual const char* getName() const = 0;

    /**
     * 프로세서 설명 반환
     */
    virtual const char* getDescription() const = 0;
};

#endif // IDURATIONPROCESSOR_H
