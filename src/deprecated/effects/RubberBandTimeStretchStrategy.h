#ifndef RUBBERBANDTIMESTRETCHSTRATEGY_H
#define RUBBERBANDTIMESTRETCHSTRATEGY_H

#include "ITimeStretchStrategy.h"

/**
 * RubberBandTimeStretchStrategy
 *
 * RubberBand Library를 사용한 최고 품질의 time stretching
 * GPL 라이센스 하에서 사용 가능 (상업적 사용은 별도 라이센스 필요)
 *
 * 장점:
 * - 최고 수준의 음질 (업계 표준)
 * - 극단적인 stretch 지원 (0.25x ~ 4.0x)
 * - Transient preservation 우수
 * - Formant preservation 지원
 * - Phase coherence 유지
 *
 * 단점:
 * - 계산 비용 매우 높음 (가장 느림)
 * - GPL 라이센스 (상업적 사용 제한)
 * - 메모리 사용량 높음
 *
 * 사용 권장:
 * - 최고 품질이 필요한 경우
 * - 극단적인 stretch ratio (>2.0x 또는 <0.5x)
 * - 오프라인 처리 (실시간 처리 불가)
 * - GPL 라이센스 수용 가능한 경우
 */
class RubberBandTimeStretchStrategy : public ITimeStretchStrategy {
public:
    RubberBandTimeStretchStrategy();
    ~RubberBandTimeStretchStrategy() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;
    const char* getName() const override { return "RubberBand Time Stretch"; }
};

#endif // RUBBERBANDTIMESTRETCHSTRATEGY_H
