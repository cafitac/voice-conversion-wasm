#ifndef SOUNDTOUCHPITCHPROCESSOR_H
#define SOUNDTOUCHPITCHPROCESSOR_H

#include "IPitchProcessor.h"
#include "../../algorithm/pitch/SoundTouchAlgorithm.h"
#include <memory>

/**
 * SoundTouchPitchProcessor
 *
 * SoundTouch 라이브러리 기반 Frame-by-Frame Pitch 프로세서
 *
 * 특징:
 * - Frame-by-frame으로 variable pitch 근사
 * - 검증된 라이브러리 (안정적)
 * - 중간 품질
 * - 200ms 프레임 + crossfade
 *
 * 동작 원리:
 * 1. 오디오를 200ms 프레임으로 분할
 * 2. 각 프레임의 평균 pitchSemitones 계산
 * 3. SoundTouch로 각 프레임 처리
 * 4. Crossfade로 경계 부드럽게
 *
 * 장점:
 * - 안정적 (프로덕션 검증됨)
 * - 빠름 (Phase Vocoder보다)
 * - Anti-aliasing 지원
 *
 * 단점:
 * - 진정한 variable pitch 아님 (200ms 단위)
 * - 프레임 경계에서 약간의 artifacts 가능
 */
class SoundTouchPitchProcessor : public IPitchProcessor {
public:
    /**
     * @param windowSize FFT 윈도우 크기 (기본 2048)
     * @param hopSize 홉 크기 (기본 512)
     */
    SoundTouchPitchProcessor(
        int windowSize = 2048,
        int hopSize = 512
    );

    ~SoundTouchPitchProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    bool supportsVariablePitch() const override { return false; }  // frame-by-frame

    const char* getName() const override {
        return "SoundTouch Pitch Processor";
    }

    const char* getDescription() const override {
        return "Stable, production-tested, frame-by-frame variable pitch";
    }

private:
    std::unique_ptr<SoundTouchAlgorithm> algorithm_;
    float frameDuration_;
    bool antiAliasing_;

    /**
     * Frame-by-frame 처리
     */
    std::vector<float> processFrameByFrame(
        const std::vector<float>& audio,
        const std::vector<FrameData>& frames,
        int sampleRate
    );

    float getPitchSemitonesAtTime(float time, const std::vector<FrameData>& frames) const;
    std::vector<float> framesToAudio(const std::vector<FrameData>& frames) const;
    std::vector<FrameData> audioToFrames(
        const std::vector<float>& audio,
        const std::vector<FrameData>& originalFrames,
        int sampleRate
    ) const;
};

#endif // SOUNDTOUCHPITCHPROCESSOR_H
