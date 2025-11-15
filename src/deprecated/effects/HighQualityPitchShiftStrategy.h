#ifndef HIGHQUALITYPITCHSHIFTSTRATEGY_H
#define HIGHQUALITYPITCHSHIFTSTRATEGY_H

#include "IPitchShiftStrategy.h"
#include "PhaseVocoderPitchShifter.h"
#include <memory>

/**
 * HighQualityPitchShiftStrategy
 *
 * 고품질 pitch shifting 전략 (Phase Vocoder)
 * PhaseVocoderPitchShifter를 래핑
 *
 * 장점:
 * - 우수한 음질 (RMS 오차 21.4% 개선)
 * - Phase coherence 유지 (phasey/smeared 소리 제거)
 * - Formant preservation (자연스러운 음성)
 * - Anti-aliasing (고주파 노이즈 감소)
 *
 * 단점:
 * - 처리 시간 증가 (~27 ms for 2초 오디오)
 * - 복잡한 구현
 *
 * 사용 권장:
 * - 음질이 중요한 경우
 * - 웹 앱, 오디오 편집 등
 */
class HighQualityPitchShiftStrategy : public IPitchShiftStrategy {
public:
    /**
     * @param fftSize FFT 크기 (기본 1024)
     * @param hopSize Hop 크기 (기본 256, 75% overlap)
     */
    HighQualityPitchShiftStrategy(int fftSize = 1024, int hopSize = 256);
    ~HighQualityPitchShiftStrategy() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;
    const char* getName() const override { return "HighQualityPitchShift (Phase Vocoder)"; }

    /**
     * Formant preservation 설정
     */
    void setFormantPreservation(bool enabled);

    /**
     * Anti-aliasing 설정
     */
    void setAntiAliasing(bool enabled);

private:
    std::unique_ptr<PhaseVocoderPitchShifter> shifter_;
};

#endif // HIGHQUALITYPITCHSHIFTSTRATEGY_H
