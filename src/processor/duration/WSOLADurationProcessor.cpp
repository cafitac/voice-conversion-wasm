#include "WSOLADurationProcessor.h"
#include <algorithm>

WSOLADurationProcessor::WSOLADurationProcessor(int windowSize, int hopSize)
    : algorithm_(new WSOLAAlgorithm(windowSize, hopSize)),
      windowSize_(windowSize),
      hopSize_(hopSize) {
}

WSOLADurationProcessor::~WSOLADurationProcessor() {
}

std::vector<FrameData> WSOLADurationProcessor::process(
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    if (frames.empty()) {
        return frames;
    }

    // FrameData → 연속 오디오 변환
    std::vector<float> audio = framesToAudio(frames);
    if (audio.empty()) {
        return frames;
    }

    // Frame-by-frame variable time stretch
    std::vector<float> processedAudio = processFrameByFrame(audio, frames, sampleRate);

    // 처리된 오디오 → FrameData 변환
    return audioToFrames(processedAudio, frames, sampleRate);
}

std::vector<float> WSOLADurationProcessor::processFrameByFrame(
    const std::vector<float>& audio,
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    // 200ms 프레임으로 분할
    const float frameDuration = 0.2f;
    const int frameSamples = static_cast<int>(frameDuration * sampleRate);
    const int overlap = frameSamples / 2;

    std::vector<float> output;
    output.reserve(audio.size() * 2);

    int frameStart = 0;

    while (frameStart < static_cast<int>(audio.size())) {
        int frameEnd = std::min(frameStart + frameSamples, static_cast<int>(audio.size()));
        std::vector<float> frameData(audio.begin() + frameStart, audio.begin() + frameEnd);

        // 이 프레임의 중앙 시간
        float frameTime = static_cast<float>(frameStart + frameSamples / 2) / sampleRate;
        float ratio = getDurationRatioAtTime(frameTime, frames);

        // WSOLA로 time stretch 적용
        AudioBuffer inputBuffer(sampleRate, 1);
        inputBuffer.setData(frameData);

        AudioBuffer outputBuffer = algorithm_->stretch(inputBuffer, ratio);
        const auto& outputData = outputBuffer.getData();

        // Output에 추가
        output.insert(output.end(), outputData.begin(), outputData.end());

        frameStart += overlap;
    }

    return output;
}

float WSOLADurationProcessor::getDurationRatioAtTime(
    float time,
    const std::vector<FrameData>& frames
) const {
    if (frames.empty()) return 1.0f;

    // 가장 가까운 프레임 찾기
    const FrameData* before = nullptr;
    const FrameData* after = nullptr;

    for (const auto& frame : frames) {
        if (frame.time <= time) before = &frame;
        if (frame.time >= time && !after) {
            after = &frame;
            break;
        }
    }

    // 선형 보간
    if (before && after && before != after) {
        float t = (time - before->time) / (after->time - before->time);
        return before->durationRatio + t * (after->durationRatio - before->durationRatio);
    }

    if (before) return before->durationRatio;
    if (after) return after->durationRatio;
    return 1.0f;
}

std::vector<float> WSOLADurationProcessor::framesToAudio(
    const std::vector<FrameData>& frames
) const {
    std::vector<float> audio;
    for (const auto& frame : frames) {
        audio.insert(audio.end(), frame.samples.begin(), frame.samples.end());
    }
    return audio;
}

std::vector<FrameData> WSOLADurationProcessor::audioToFrames(
    const std::vector<float>& audio,
    const std::vector<FrameData>& originalFrames,
    int sampleRate
) const {
    if (originalFrames.empty()) return {};

    std::vector<FrameData> result;
    size_t audioOffset = 0;

    for (const auto& originalFrame : originalFrames) {
        FrameData newFrame = originalFrame;
        size_t frameSize = originalFrame.samples.size();
        newFrame.samples.clear();

        for (size_t i = 0; i < frameSize && audioOffset < audio.size(); ++i) {
            newFrame.samples.push_back(audio[audioOffset++]);
        }

        while (newFrame.samples.size() < frameSize) {
            newFrame.samples.push_back(0.0f);
        }

        result.push_back(newFrame);
    }

    return result;
}
