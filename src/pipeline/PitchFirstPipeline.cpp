#include "PitchFirstPipeline.h"
#include <algorithm>
#include <cmath>

PitchFirstPipeline::PitchFirstPipeline(float gradientThreshold, float frameInterval)
    : outlierCorrector_(new OutlierCorrector(gradientThreshold, 2, 3)),
      splineInterpolator_(new SplineInterpolator(frameInterval, 48000)),
      reconstructor_(new FrameReconstructor()),
      gradientThreshold_(gradientThreshold),
      frameInterval_(frameInterval) {
}

PitchFirstPipeline::~PitchFirstPipeline() {
}

std::vector<FrameData> PitchFirstPipeline::preprocessOnly(
    const std::vector<FrameData>& editPoints,
    float totalDuration,
    int sampleRate
) {
    if (editPoints.empty()) {
        return {};
    }

    // 편집 포인트들은 isEdited = true로 표시되어 있어야 함
    std::vector<FrameData> frames = editPoints;

    // 모든 편집 포인트를 isEdited = true로 표시
    for (auto& frame : frames) {
        frame.isEdited = true;
    }

    // SplineInterpolator에 totalDuration 전달
    splineInterpolator_->setTotalDuration(totalDuration);
    splineInterpolator_->setSampleRate(sampleRate);

    // 전처리 체인 실행
    frames = runPreprocessors(frames);

    return frames;
}

std::vector<FrameData> PitchFirstPipeline::runPreprocessors(
    const std::vector<FrameData>& frames
) {
    // 1. Outlier Correction (gradient 기반)
    std::vector<FrameData> corrected = outlierCorrector_->process(frames);

    // 2. Spline Interpolation (cubic spline)
    std::vector<FrameData> interpolated = splineInterpolator_->process(corrected);

    return interpolated;
}

AudioBuffer PitchFirstPipeline::execute(
    const std::vector<float>& audioData,
    const std::vector<FrameData>& frames,
    int sampleRate,
    IPitchProcessor* pitchProcessor,
    IDurationProcessor* durationProcessor
) {
    if (audioData.empty() || frames.empty()) {
        return AudioBuffer(sampleRate, 1);
    }

    if (!pitchProcessor) {
        // Pitch processor가 없으면 원본 반환
        AudioBuffer result(sampleRate, 1);
        result.setData(audioData);
        return result;
    }

    // 1. 원본 오디오를 FrameData에 채움
    std::vector<FrameData> framesWithAudio = populateAudioSamples(audioData, frames, sampleRate);

    // 2. Pitch 처리
    std::vector<FrameData> pitchedFrames = pitchProcessor->process(framesWithAudio, sampleRate);

    // 3. Duration 처리 (옵션)
    std::vector<FrameData> finalFrames;
    if (durationProcessor) {
        finalFrames = durationProcessor->process(pitchedFrames, sampleRate);
    } else {
        finalFrames = pitchedFrames;
    }

    // 4. FrameData → AudioBuffer 재구성
    // Reconstructor는 overlap-add로 FrameData를 AudioBuffer로 변환
    AudioBuffer result = reconstructor_->reconstruct(
        finalFrames,
        sampleRate,
        1,  // mono
        frameInterval_,
        std::vector<float>()  // timeRatios (비어있으면 1.0 사용)
    );

    return result;
}

std::vector<FrameData> PitchFirstPipeline::populateAudioSamples(
    const std::vector<float>& audioData,
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    std::vector<FrameData> result;

    int samplesPerFrame = static_cast<int>(frameInterval_ * sampleRate);

    for (const auto& frame : frames) {
        FrameData newFrame = frame;  // 메타데이터 복사

        // 이 프레임의 시작 샘플 인덱스
        int startSample = static_cast<int>(frame.time * sampleRate);
        int endSample = std::min(startSample + samplesPerFrame, static_cast<int>(audioData.size()));

        // 오디오 샘플 복사
        newFrame.samples.clear();
        for (int i = startSample; i < endSample; ++i) {
            if (i >= 0 && i < static_cast<int>(audioData.size())) {
                newFrame.samples.push_back(audioData[i]);
            }
        }

        // 프레임이 부족하면 0으로 채움
        while (static_cast<int>(newFrame.samples.size()) < samplesPerFrame) {
            newFrame.samples.push_back(0.0f);
        }

        result.push_back(newFrame);
    }

    return result;
}

void PitchFirstPipeline::setGradientThreshold(float threshold) {
    gradientThreshold_ = threshold;
    outlierCorrector_->setGradientThreshold(threshold);
}

void PitchFirstPipeline::setFrameInterval(float interval) {
    frameInterval_ = interval;
    splineInterpolator_->setFrameInterval(interval);
}
