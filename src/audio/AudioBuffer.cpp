#include "AudioBuffer.h"

AudioBuffer::AudioBuffer()
    : sampleRate_(44100), channels_(1) {
}

AudioBuffer::AudioBuffer(int sampleRate, int channels)
    : sampleRate_(sampleRate), channels_(channels) {
}

AudioBuffer::~AudioBuffer() {
}

void AudioBuffer::setData(const std::vector<float>& data) {
    data_ = data;
}

void AudioBuffer::appendData(const std::vector<float>& data) {
    data_.insert(data_.end(), data.begin(), data.end());
}

void AudioBuffer::clear() {
    data_.clear();
}

const std::vector<float>& AudioBuffer::getData() const {
    return data_;
}

std::vector<float>& AudioBuffer::getData() {
    return data_;
}

int AudioBuffer::getSampleRate() const {
    return sampleRate_;
}

int AudioBuffer::getChannels() const {
    return channels_;
}

size_t AudioBuffer::getLength() const {
    return data_.size();
}

float AudioBuffer::getDuration() const {
    if (sampleRate_ == 0 || channels_ == 0) return 0.0f;
    return static_cast<float>(data_.size()) / (sampleRate_ * channels_);
}

void AudioBuffer::setSampleRate(int rate) {
    sampleRate_ = rate;
}

void AudioBuffer::setChannels(int channels) {
    channels_ = channels;
}
