#ifndef RUBBERBANDALGORITHM_H
#define RUBBERBANDALGORITHM_H

#include "IPitchAlgorithm.h"
#include "../../external/rubberband/rubberband/RubberBandStretcher.h"
#include <memory>

/**
 * RubberBandAlgorithm
 *
 * RubberBand 라이브러리 기반 pitch shifting 알고리즘
 *
 * 특징:
 * - 최고 품질의 검증된 라이브러리
 * - Formant preservation 지원
 * - GPL 라이선스 (상업적 사용 주의)
 *
 * 동작 원리:
 * - 고급 phase vocoder + transient detection
 * - Time-frequency analysis
 *
 * 장점:
 * - 최고 품질
 * - Formant preservation
 * - 프로덕션 검증됨 (Audacity 등 사용)
 *
 * 단점:
 * - GPL 라이선스 (상업적 사용 제한)
 * - SoundTouch보다 느림
 */
class RubberBandAlgorithm : public IPitchAlgorithm {
public:
    /**
     * @param preserveFormant Formant preservation (기본 true)
     * @param highQuality 고품질 모드 (기본 true)
     */
    RubberBandAlgorithm(
        bool preserveFormant = true,
        bool highQuality = true
    );

    ~RubberBandAlgorithm() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;

    const char* getName() const override {
        return "RubberBand";
    }

    const char* getDescription() const override {
        return "Highest quality, formant preservation, GPL license";
    }

    bool supportsFormantPreservation() const override { return true; }

private:
    bool preserveFormant_;
    bool highQuality_;
};

#endif // RUBBERBANDALGORITHM_H
