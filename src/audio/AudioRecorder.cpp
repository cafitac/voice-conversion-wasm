#include "AudioRecorder.h"
#include <cstring>
#include <emscripten/emscripten.h>

AudioRecorder::AudioRecorder(int sampleRate, int channels)
    : buffer_(std::make_unique<AudioBuffer>(sampleRate, channels)),
      isRecording_(false) {
}

AudioRecorder::~AudioRecorder() {
}

void AudioRecorder::addAudioData(uintptr_t dataPtr, int length) {
    if (!isRecording_) return;

    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    // 무음 청크 감지 (모든 샘플이 임계값 이하인지 확인)
    bool isSilent = true;
    const float silenceThreshold = 0.001f;
    for (int i = 0; i < length; i++) {
        if (std::abs(data[i]) > silenceThreshold) {
            isSilent = false;
            break;
        }
    }

    // 버퍼가 비어있고 무음 청크면 건너뛰기 (녹음 시작 시 무음 제거)
    if (buffer_->getData().empty() && isSilent) {
        return;
    }

    buffer_->appendData(samples);
}

void AudioRecorder::startRecording() {
    isRecording_ = true;
    buffer_->clear();
}

void AudioRecorder::stopRecording() {
    isRecording_ = false;
}

bool AudioRecorder::isRecording() const {
    return isRecording_;
}

const AudioBuffer& AudioRecorder::getRecordedAudio() const {
    return *buffer_;
}

void AudioRecorder::clearRecording() {
    buffer_->clear();
}
