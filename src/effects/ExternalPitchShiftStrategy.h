#ifndef EXTERNALPITCHSHIFTSTRATEGY_H
#define EXTERNALPITCHSHIFTSTRATEGY_H

#include "IPitchShiftStrategy.h"
#include "../external/soundtouch/include/SoundTouch.h"
#include <memory>

/**
 * ExternalPitchShiftStrategy
 *
 * 외부 라이브러리(SoundTouch)를 사용한 고품질 pitch shifting 전략
 *
 * 장점:
 * - 검증된 프로덕션 라이브러리 (업계 표준)
 * - 뛰어난 음질 (Formant preservation, Anti-aliasing)
 * - 최적화된 성능 (SIMD 지원)
 * - 노이즈 제거 및 Phase coherence 자동 처리
 *
 * 단점:
 * - 외부 의존성 추가
 * - 라이브러리 크기 (웹 앱의 경우 WASM 크기 증가)
 *
 * 사용 권장:
 * - 프로덕션 환경에서 최고 품질이 필요한 경우
 * - 음성/음악 편집, 방송, 녹음 스튜디오 등
 * - 외부 의존성을 허용할 수 있는 경우
 */
class ExternalPitchShiftStrategy : public IPitchShiftStrategy {
public:
    /**
     * @param antiAliasing Anti-alias filter 활성화 (기본 true)
     * @param quickSeek Quick seeking 활성화 (속도 향상, 약간의 품질 저하, 기본 false)
     */
    ExternalPitchShiftStrategy(bool antiAliasing = true, bool quickSeek = false);
    ~ExternalPitchShiftStrategy() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;
    const char* getName() const override { return "ExternalPitchShift (SoundTouch)"; }

    /**
     * Anti-aliasing filter 설정
     */
    void setAntiAliasing(bool enabled);

    /**
     * Quick seek 알고리즘 설정
     * (CPU 사용량 감소, 약간의 품질 저하)
     */
    void setQuickSeek(bool enabled);

private:
    std::unique_ptr<soundtouch::SoundTouch> soundtouch_;
    bool antiAliasing_;
    bool quickSeek_;
};

#endif // EXTERNALPITCHSHIFTSTRATEGY_H
