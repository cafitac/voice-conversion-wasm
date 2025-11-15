#ifndef PITCHFIRSTPIPELINE_H
#define PITCHFIRSTPIPELINE_H

#include "IPipeline.h"
#include "../preprocessor/OutlierCorrector.h"
#include "../preprocessor/SplineInterpolator.h"
#include "../audio/FrameReconstructor.h"
#include <memory>

/**
 * PitchFirstPipeline
 *
 * Pitch → Duration 순서로 처리하는 파이프라인
 *
 * 처리 흐름:
 * 1. 전처리 체인
 *    - OutlierCorrector: gradient 기반 극단값 보정
 *    - SplineInterpolator: cubic spline 보간
 * 2. Pitch Processor: 선택된 알고리즘으로 pitch shift
 * 3. Duration Processor: 선택된 알고리즘으로 time stretch (옵션)
 * 4. Reconstructor: FrameData → AudioBuffer 변환
 *
 * 특징:
 * - 음성 편집에 적합 (pitch를 먼저 조정)
 * - 대부분의 경우 권장되는 순서
 */
class PitchFirstPipeline : public IPipeline {
public:
    /**
     * @param gradientThreshold Outlier 검출 threshold (기본 3.0 semitones)
     * @param frameInterval 프레임 간격 (기본 0.02초 = 20ms)
     */
    PitchFirstPipeline(
        float gradientThreshold = 3.0f,
        float frameInterval = 0.02f
    );

    ~PitchFirstPipeline() override;

    std::vector<FrameData> preprocessOnly(
        const std::vector<FrameData>& editPoints,
        float totalDuration,
        int sampleRate
    ) override;

    AudioBuffer execute(
        const std::vector<float>& audioData,
        const std::vector<FrameData>& frames,
        int sampleRate,
        IPitchProcessor* pitchProcessor,
        IDurationProcessor* durationProcessor
    ) override;

    const char* getName() const override {
        return "Pitch-First Pipeline";
    }

    const char* getDescription() const override {
        return "Process pitch first, then duration (recommended for voice)";
    }

    /**
     * Outlier corrector 설정
     */
    void setGradientThreshold(float threshold);

    /**
     * Frame interval 설정
     */
    void setFrameInterval(float interval);

protected:
    std::vector<FrameData> runPreprocessors(
        const std::vector<FrameData>& frames
    ) override;

private:
    std::unique_ptr<OutlierCorrector> outlierCorrector_;
    std::unique_ptr<SplineInterpolator> splineInterpolator_;
    std::unique_ptr<FrameReconstructor> reconstructor_;

    float gradientThreshold_;
    float frameInterval_;

    /**
     * 원본 오디오에서 FrameData 생성
     *
     * @param audioData 원본 오디오 샘플
     * @param frames 보간된 FrameData (메타데이터용)
     * @param sampleRate 샘플 레이트
     * @return 오디오 샘플이 채워진 FrameData
     */
    std::vector<FrameData> populateAudioSamples(
        const std::vector<float>& audioData,
        const std::vector<FrameData>& frames,
        int sampleRate
    );
};

#endif // PITCHFIRSTPIPELINE_H
