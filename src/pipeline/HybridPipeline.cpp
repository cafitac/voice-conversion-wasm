#include "HybridPipeline.h"

HybridPipeline::HybridPipeline(bool previewMode, float gradientThreshold, float frameInterval)
    : PitchFirstPipeline(gradientThreshold, frameInterval),
      previewMode_(previewMode),
      psolaProcessor_(new PSOLAPitchProcessor(2048, 512)),
      phaseVocoderProcessor_(new PhaseVocoderPitchProcessor(2048, 512, true)) {
}

HybridPipeline::~HybridPipeline() {
}

AudioBuffer HybridPipeline::execute(
    const std::vector<float>& audioData,
    const std::vector<FrameData>& frames,
    int sampleRate,
    IPitchProcessor* pitchProcessor,  // 무시됨
    IDurationProcessor* durationProcessor
) {
    // previewMode에 따라 내부 프로세서 선택
    IPitchProcessor* selectedProcessor = previewMode_
        ? static_cast<IPitchProcessor*>(psolaProcessor_.get())
        : static_cast<IPitchProcessor*>(phaseVocoderProcessor_.get());

    // 부모 클래스의 execute() 호출 (내부 프로세서 사용)
    return PitchFirstPipeline::execute(
        audioData,
        frames,
        sampleRate,
        selectedProcessor,
        durationProcessor
    );
}

void HybridPipeline::setPreviewMode(bool previewMode) {
    previewMode_ = previewMode;
}
