#ifndef ITIMESTRETCHSTRATEGY_H
#define ITIMESTRETCHSTRATEGY_H

#include "../audio/AudioBuffer.h"

/**
 * ITimeStretchStrategy
 *
 * Time stretching 알고리즘을 위한 Strategy Pattern 인터페이스
 * 런타임에 알고리즘을 교체 가능하도록 설계
 *
 * 사용 예:
 *   - FastTimeStretchStrategy: 빠른 처리 (낮은 품질, frame 복제/삭제)
 *   - HighQualityTimeStretchStrategy: 고품질 처리 (WSOLA)
 *   - ExternalTimeStretchStrategy: 외부 라이브러리 (SoundTouch)
 */
class ITimeStretchStrategy {
public:
    virtual ~ITimeStretchStrategy() = default;

    /**
     * Time stretch 적용
     *
     * @param input 입력 오디오 버퍼
     * @param ratio Time stretch ratio (1.0 = 원본, >1.0 = 느리게, <1.0 = 빠르게)
     * @return Time stretched 오디오 버퍼
     */
    virtual AudioBuffer stretch(const AudioBuffer& input, float ratio) = 0;

    /**
     * 전략 이름 반환 (디버깅/로깅용)
     */
    virtual const char* getName() const = 0;
};

#endif // ITIMESTRETCHSTRATEGY_H
