#ifndef FASTTIMESTRETCHSTRATEGY_H
#define FASTTIMESTRETCHSTRATEGY_H

#include "ITimeStretchStrategy.h"

/**
 * FastTimeStretchStrategy
 *
 * 간단한 frame 복제/삭제 방식의 time stretching
 *
 * 장점:
 * - 매우 빠른 처리 속도
 * - 단순한 구현
 * - 메모리 효율적
 *
 * 단점:
 * - 낮은 음질 (클릭 노이즈, 불연속점)
 * - Phase coherence 없음
 * - 음악/음성 왜곡 발생
 *
 * 사용 권장:
 * - 실시간 처리가 필수인 경우
 * - 음질보다 속도가 중요한 경우
 * - 프로토타입/테스트 목적
 */
class FastTimeStretchStrategy : public ITimeStretchStrategy {
public:
    FastTimeStretchStrategy();
    ~FastTimeStretchStrategy() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;
    const char* getName() const override { return "FastTimeStretch (Frame Repeat/Skip)"; }

private:
    /**
     * Frame 단위로 복제/삭제하여 time stretch 수행
     */
    std::vector<float> simpleFrameStretch(const std::vector<float>& input,
                                          float ratio,
                                          int sampleRate,
                                          int channels);
};

#endif // FASTTIMESTRETCHSTRATEGY_H
