#include "SoundTouchPitchProcessor.h"
#include <cmath>
#include <algorithm>

SoundTouchPitchProcessor::SoundTouchPitchProcessor(int windowSize, int hopSize)
    : algorithm_(new SoundTouchAlgorithm(true, false)),
      frameDuration_(0.2f),  // 200ms default
      antiAliasing_(true) {
    // windowSize와 hopSize는 내부적으로 사용하지 않음 (SoundTouch가 자체 처리)
    // frameDuration_은 frame-by-frame 처리 시 프레임 길이로 사용
}

SoundTouchPitchProcessor::~SoundTouchPitchProcessor() {
}

std::vector<FrameData> SoundTouchPitchProcessor::process(
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    if (frames.empty()) {
        return frames;
    }

    std::vector<float> audio = framesToAudio(frames);
    if (audio.empty()) {
        return frames;
    }

    std::vector<float> processedAudio = processFrameByFrame(audio, frames, sampleRate);
    return audioToFrames(processedAudio, frames, sampleRate);
}

std::vector<float> SoundTouchPitchProcessor::processFrameByFrame(
    const std::vector<float>& audio,
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    const int frameSamples = static_cast<int>(frameDuration_ * sampleRate);
    const int overlap = frameSamples / 2;  // 50% overlap

    std::vector<float> output(audio.size() * 2, 0.0f);
    std::vector<int> overlapCount(output.size(), 0);

    int frameStart = 0;

    while (frameStart < static_cast<int>(audio.size())) {
        int frameEnd = std::min(frameStart + frameSamples, static_cast<int>(audio.size()));

        std::vector<float> frameData(audio.begin() + frameStart, audio.begin() + frameEnd);

        float frameTime = static_cast<float>(frameStart + frameSamples / 2) / sampleRate;
        float semitones = getPitchSemitonesAtTime(frameTime, frames);

        AudioBuffer inputBuffer(sampleRate, 1);
        inputBuffer.setData(frameData);

        AudioBuffer outputBuffer = algorithm_->shiftPitch(inputBuffer, semitones);
        const auto& outputData = outputBuffer.getData();

        int outputStart = frameStart;
        for (size_t i = 0; i < outputData.size() && outputStart + i < output.size(); ++i) {
            output[outputStart + i] += outputData[i];
            overlapCount[outputStart + i]++;
        }

        frameStart += overlap;
    }

    for (size_t i = 0; i < output.size(); ++i) {
        if (overlapCount[i] > 0) {
            output[i] /= overlapCount[i];
        }
    }

    output.resize(std::min(output.size(), audio.size() * 2));
    return output;
}

float SoundTouchPitchProcessor::getPitchSemitonesAtTime(
    float time,
    const std::vector<FrameData>& frames
) const {
    if (frames.empty()) return 0.0f;

    const FrameData* before = nullptr;
    const FrameData* after = nullptr;

    for (const auto& frame : frames) {
        if (frame.time <= time) before = &frame;
        if (frame.time >= time && !after) {
            after = &frame;
            break;
        }
    }

    if (before && after && before != after) {
        float t = (time - before->time) / (after->time - before->time);
        return before->pitchSemitones + t * (after->pitchSemitones - before->pitchSemitones);
    }

    if (before) return before->pitchSemitones;
    if (after) return after->pitchSemitones;
    return 0.0f;
}

std::vector<float> SoundTouchPitchProcessor::framesToAudio(
    const std::vector<FrameData>& frames
) const {
    std::vector<float> audio;
    for (const auto& frame : frames) {
        audio.insert(audio.end(), frame.samples.begin(), frame.samples.end());
    }
    return audio;
}

std::vector<FrameData> SoundTouchPitchProcessor::audioToFrames(
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
