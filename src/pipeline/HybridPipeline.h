#ifndef HYBRIDPIPELINE_H
#define HYBRIDPIPELINE_H

#include "PitchFirstPipeline.h"
#include "../processor/pitch/PSOLAPitchProcessor.h"
#include "../processor/pitch/PhaseVocoderPitchProcessor.h"

/**
 * HybridPipeline
 *
 * 미리듣기와 최종 생성에 다른 알고리즘을 사용하는 파이프라인
 *
 * 처리 모드:
 * - Preview Mode (previewMode = true):
 *   PSOLA 사용 → 빠른 처리, 실시간 미리듣기
 *   사용자가 "샘플 생성" 버튼 클릭 시
 *
 * - Final Mode (previewMode = false):
 *   Phase Vocoder 사용 → 최고 품질
 *   사용자가 "최종 생성" 버튼 클릭 시
 *
 * 특징:
 * - 빠른 피드백: PSOLA로 즉시 미리듣기
 * - 높은 품질: Phase Vocoder로 최종 결과
 * - 사용자 경험 최적화
 *
 * 사용 시나리오:
 * 1. 사용자가 그래프에서 pitch 편집
 * 2. "샘플 생성" 클릭 → PSOLA (1~2초)
 * 3. 결과 확인하고 수정
 * 4. "최종 생성" 클릭 → Phase Vocoder (5~10초)
 * 5. 최고 품질 결과 다운로드
 */
class HybridPipeline : public PitchFirstPipeline {
public:
    /**
     * @param previewMode true = PSOLA (빠름), false = Phase Vocoder (고품질)
     * @param gradientThreshold Outlier 검출 threshold
     * @param frameInterval 프레임 간격
     */
    HybridPipeline(
        bool previewMode = true,
        float gradientThreshold = 3.0f,
        float frameInterval = 0.02f
    );

    ~HybridPipeline() override;

    /**
     * 전체 파이프라인 실행 (Hybrid 모드)
     *
     * previewMode에 따라 다른 프로세서 사용:
     * - true: PSOLA (빠름, 미리듣기)
     * - false: Phase Vocoder (고품질, 최종)
     *
     * 전달된 pitchProcessor는 무시되고 내부 프로세서 사용
     */
    AudioBuffer execute(
        const std::vector<float>& audioData,
        const std::vector<FrameData>& frames,
        int sampleRate,
        IPitchProcessor* pitchProcessor,  // 무시됨
        IDurationProcessor* durationProcessor
    ) override;

    const char* getName() const override {
        return previewMode_ ? "Hybrid Pipeline (Preview)" : "Hybrid Pipeline (Final)";
    }

    const char* getDescription() const override {
        return previewMode_
            ? "Fast preview with PSOLA"
            : "High quality final with Phase Vocoder";
    }

    /**
     * Preview/Final 모드 전환
     */
    void setPreviewMode(bool previewMode);

    /**
     * 현재 모드 확인
     */
    bool isPreviewMode() const { return previewMode_; }

private:
    bool previewMode_;
    std::unique_ptr<PSOLAPitchProcessor> psolaProcessor_;
    std::unique_ptr<PhaseVocoderPitchProcessor> phaseVocoderProcessor_;
};

#endif // HYBRIDPIPELINE_H
