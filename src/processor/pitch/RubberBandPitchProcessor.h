#ifndef RUBBERBANDPITCHPROCESSOR_H
#define RUBBERBANDPITCHPROCESSOR_H

#include "IPitchProcessor.h"
#include "../../algorithm/pitch/RubberBandAlgorithm.h"
#include <memory>

/**
 * RubberBandPitchProcessor
 *
 * RubberBand 라이브러리 기반 Frame-by-Frame Pitch 프로세서
 *
 * 특징:
 * - Frame-by-frame으로 variable pitch 근사
 * - 최고 품질 라이브러리
 * - 100ms 프레임 + crossfade
 * - Formant preservation 지원
 *
 * 동작 원리:
 * 1. 오디오를 100ms 프레임으로 분할 (SoundTouch보다 짧음)
 * 2. 각 프레임의 평균 pitchSemitones 계산
 * 3. RubberBand로 각 프레임 처리
 * 4. Crossfade로 경계 부드럽게
 *
 * 장점:
 * - 최고 품질
 * - Formant preservation
 * - 프로덕션 검증됨
 *
 * 단점:
 * - GPL 라이선스 (상업적 사용 주의)
 * - 진정한 variable pitch 아님 (100ms 단위)
 * - 느림 (SoundTouch보다)
 */
class RubberBandPitchProcessor : public IPitchProcessor {
public:
    /**
     * @param windowSize FFT 윈도우 크기 (기본 2048)
     * @param hopSize 홉 크기 (기본 512)
     */
    RubberBandPitchProcessor(
        int windowSize = 2048,
        int hopSize = 512
    );

    ~RubberBandPitchProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    bool supportsVariablePitch() const override { return false; }  // frame-by-frame

    const char* getName() const override {
        return "RubberBand Pitch Processor";
    }

    const char* getDescription() const override {
        return "Highest quality, production-grade, frame-by-frame variable pitch";
    }

private:
    std::unique_ptr<RubberBandAlgorithm> algorithm_;
    float frameDuration_;
    bool preserveFormant_;

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

#endif // RUBBERBANDPITCHPROCESSOR_H
