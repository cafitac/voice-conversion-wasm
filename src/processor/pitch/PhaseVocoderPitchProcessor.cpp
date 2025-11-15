#include "PhaseVocoderPitchProcessor.h"
#include <cmath>
#include <algorithm>

PhaseVocoderPitchProcessor::PhaseVocoderPitchProcessor(
    int fftSize,
    int hopSize,
    bool formantPreservation
)
    : shifter_(new PhaseVocoderPitchShifter(fftSize, hopSize)),
      fftSize_(fftSize),
      hopSize_(hopSize),
      formantPreservation_(formantPreservation) {

    shifter_->setFormantPreservation(formantPreservation_);
}

PhaseVocoderPitchProcessor::~PhaseVocoderPitchProcessor() {
}

std::vector<FrameData> PhaseVocoderPitchProcessor::process(
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    if (frames.empty()) {
        return frames;
    }

    // 1. FrameData → 연속 오디오 변환
    std::vector<float> audio = framesToAudio(frames);

    if (audio.empty()) {
        return frames;
    }

    // 2. Variable pitch shift 적용
    std::vector<float> processedAudio = processVariablePitch(audio, frames, sampleRate);

    // 3. 처리된 오디오 → FrameData 변환
    std::vector<FrameData> result = audioToFrames(processedAudio, frames, sampleRate);

    return result;
}

std::vector<float> PhaseVocoderPitchProcessor::processVariablePitch(
    const std::vector<float>& audio,
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    // Frame-by-frame 처리 (variable pitch 근사)
    // 각 프레임(0.1초)마다 다른 pitch shift 적용

    const float frameDuration = 0.1f;  // 100ms
    const int frameSamples = static_cast<int>(frameDuration * sampleRate);
    const int overlap = frameSamples / 2;  // 50% overlap

    std::vector<float> output(audio.size() * 2, 0.0f);  // 충분한 크기
    std::vector<int> overlapCount(output.size(), 0);

    int frameStart = 0;

    while (frameStart < static_cast<int>(audio.size())) {
        int frameEnd = std::min(frameStart + frameSamples, static_cast<int>(audio.size()));

        // 현재 프레임 추출
        std::vector<float> frameData(audio.begin() + frameStart, audio.begin() + frameEnd);

        // 이 프레임의 시간
        float frameTime = static_cast<float>(frameStart) / sampleRate;

        // 이 프레임의 평균 pitchSemitones 계산
        float semitones = getPitchSemitonesAtTime(frameTime + frameDuration / 2.0f, frames);

        // Phase Vocoder로 pitch shift 적용
        AudioBuffer inputBuffer(sampleRate, 1);
        inputBuffer.setData(frameData);

        AudioBuffer outputBuffer = shifter_->shiftPitch(inputBuffer, semitones);
        const auto& outputData = outputBuffer.getData();

        // Overlap-add
        int outputStart = frameStart;
        for (size_t i = 0; i < outputData.size() && outputStart + i < output.size(); ++i) {
            output[outputStart + i] += outputData[i];
            overlapCount[outputStart + i]++;
        }

        frameStart += overlap;
    }

    // 정규화 (overlap count로 나누기)
    for (size_t i = 0; i < output.size(); ++i) {
        if (overlapCount[i] > 0) {
            output[i] /= overlapCount[i];
        }
    }

    // 실제 사용된 크기로 축소
    size_t actualSize = std::min(output.size(), static_cast<size_t>(audio.size() * 1.5f));
    output.resize(actualSize);

    return output;
}

float PhaseVocoderPitchProcessor::getPitchSemitonesAtTime(
    float time,
    const std::vector<FrameData>& frames
) const {
    if (frames.empty()) {
        return 0.0f;
    }

    // 정확히 일치하는 프레임 찾기
    for (const auto& frame : frames) {
        if (std::abs(frame.time - time) < 0.001f) {
            return frame.pitchSemitones;
        }
    }

    // 선형 보간
    const FrameData* before = nullptr;
    const FrameData* after = nullptr;

    for (size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].time <= time) {
            before = &frames[i];
        }
        if (frames[i].time >= time && after == nullptr) {
            after = &frames[i];
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

std::vector<float> PhaseVocoderPitchProcessor::framesToAudio(
    const std::vector<FrameData>& frames
) const {
    std::vector<float> audio;

    for (const auto& frame : frames) {
        audio.insert(audio.end(), frame.samples.begin(), frame.samples.end());
    }

    return audio;
}

std::vector<FrameData> PhaseVocoderPitchProcessor::audioToFrames(
    const std::vector<float>& audio,
    const std::vector<FrameData>& originalFrames,
    int sampleRate
) const {
    if (originalFrames.empty()) {
        return {};
    }

    std::vector<FrameData> result;

    // 원본 프레임 구조 유지
    size_t audioOffset = 0;
    for (const auto& originalFrame : originalFrames) {
        FrameData newFrame = originalFrame;  // 메타데이터 복사

        // 샘플 복사
        size_t frameSize = originalFrame.samples.size();
        newFrame.samples.clear();

        for (size_t i = 0; i < frameSize && audioOffset < audio.size(); ++i) {
            newFrame.samples.push_back(audio[audioOffset++]);
        }

        // 프레임이 부족하면 0으로 채움
        while (newFrame.samples.size() < frameSize) {
            newFrame.samples.push_back(0.0f);
        }

        result.push_back(newFrame);
    }

    return result;
}

void PhaseVocoderPitchProcessor::setFormantPreservation(bool enabled) {
    formantPreservation_ = enabled;
    shifter_->setFormantPreservation(enabled);
}
