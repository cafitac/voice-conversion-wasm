#ifndef PHASEVOCODERTIMESTRETCHSTRATEGY_H
#define PHASEVOCODERTIMESTRETCHSTRATEGY_H

#include "ITimeStretchStrategy.h"
#include "PhaseVocoder.h"
#include <memory>

/**
 * PhaseVocoderTimeStretchStrategy
 *
 * Phase Vocoder 알고리즘을 사용한 time stretching
 * WSOLA보다 극단적인 stretch에 강하고 품질이 우수
 *
 * 장점:
 * - 우수한 음질 (WSOLA보다 높음)
 * - 극단적인 stretch 지원 (0.5x ~ 2.0x)
 * - Phase coherence 유지
 * - 기존 코드베이스 활용 (외부 의존성 없음)
 *
 * 단점:
 * - 계산 비용 높음 (FFT 연산)
 * - WSOLA보다 느림
 * - Transient 손실 가능
 *
 * 사용 권장:
 * - 큰 stretch ratio 필요 (>1.5x 또는 <0.7x)
 * - 음질이 최우선인 경우
 * - 외부 라이브러리 사용 불가능한 경우
 */
class PhaseVocoderTimeStretchStrategy : public ITimeStretchStrategy {
public:
    /**
     * @param fftSize FFT 크기 (기본 2048)
     * @param hopSize Hop 크기 (기본 512, 75% overlap)
     */
    PhaseVocoderTimeStretchStrategy(int fftSize = 2048, int hopSize = 512);
    ~PhaseVocoderTimeStretchStrategy() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;
    const char* getName() const override { return "PhaseVocoder Time Stretch"; }

private:
    std::unique_ptr<PhaseVocoder> vocoder_;
};

#endif // PHASEVOCODERTIMESTRETCHSTRATEGY_H
