#ifndef PHASEVOCODERALGORITHM_H
#define PHASEVOCODERALGORITHM_H

#include "IPitchAlgorithm.h"
#include "../../effects/PhaseVocoderPitchShifter.h"
#include <memory>

/**
 * PhaseVocoderAlgorithm
 *
 * Phase Vocoder 기반 pitch shifting 알고리즘
 *
 * 특징:
 * - Frequency-domain 처리 (STFT/ISTFT)
 * - 최고 품질
 * - Phase coherence 유지
 * - Formant preservation 지원
 *
 * 동작 원리:
 * 1. STFT로 주파수 도메인 변환
 * 2. Frequency bin 재배치
 * 3. Phase 보정
 * 4. ISTFT로 시간 도메인 복원
 *
 * 장점:
 * - 우수한 음질
 * - Formant 왜곡 최소화
 * - 범용성 (음성, 음악 모두 가능)
 *
 * 단점:
 * - 느린 처리 속도 (5-10초)
 * - 복잡한 구현
 */
class PhaseVocoderAlgorithm : public IPitchAlgorithm {
public:
    /**
     * @param fftSize FFT 크기 (기본 2048)
     * @param hopSize Hop 크기 (기본 512)
     * @param preserveFormant Formant preservation (기본 true)
     */
    PhaseVocoderAlgorithm(
        int fftSize = 2048,
        int hopSize = 512,
        bool preserveFormant = true
    );

    ~PhaseVocoderAlgorithm() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;

    const char* getName() const override {
        return "Phase Vocoder";
    }

    const char* getDescription() const override {
        return "Highest quality, frequency-domain, formant preservation";
    }

    bool supportsFormantPreservation() const override { return true; }

    /**
     * Formant preservation 설정
     */
    void setFormantPreservation(bool enabled);

private:
    std::unique_ptr<PhaseVocoderPitchShifter> shifter_;
    bool preserveFormant_;
};

#endif // PHASEVOCODERALGORITHM_H
