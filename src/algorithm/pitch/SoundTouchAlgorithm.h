#ifndef SOUNDTOUCHALGORITHM_H
#define SOUNDTOUCHALGORITHM_H

#include "IPitchAlgorithm.h"
#include "../../external/soundtouch/include/SoundTouch.h"
#include <memory>

/**
 * SoundTouchAlgorithm
 *
 * SoundTouch 라이브러리 기반 pitch shifting 알고리즘
 *
 * 특징:
 * - 검증된 오픈소스 라이브러리
 * - 안정적인 품질
 * - 중간 수준 처리 속도
 * - LGPL 라이선스 (상업적 사용 가능)
 *
 * 동작 원리:
 * - TDStretch + Rate transposer 조합
 * - Correlation-based overlap-add
 *
 * 장점:
 * - 안정적 (프로덕션 검증됨)
 * - Anti-aliasing 지원
 * - 상업적 사용 가능 (LGPL)
 *
 * 단점:
 * - Phase Vocoder보다는 낮은 품질
 * - Formant preservation 미지원
 */
class SoundTouchAlgorithm : public IPitchAlgorithm {
public:
    /**
     * @param antiAliasing Anti-aliasing 필터 (기본 true)
     * @param quickSeek Quick seek 모드 (기본 false)
     */
    SoundTouchAlgorithm(
        bool antiAliasing = true,
        bool quickSeek = false
    );

    ~SoundTouchAlgorithm() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;

    const char* getName() const override {
        return "SoundTouch";
    }

    const char* getDescription() const override {
        return "Stable, production-tested, LGPL license";
    }

    bool supportsRealtime() const override { return true; }

private:
    std::unique_ptr<soundtouch::SoundTouch> soundTouch_;
    bool antiAliasing_;
    bool quickSeek_;
};

#endif // SOUNDTOUCHALGORITHM_H
