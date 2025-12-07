#include "AudioReverser.h"
#include <algorithm>

AudioReverser::AudioReverser() {
}

AudioReverser::~AudioReverser() {
}

AudioBuffer AudioReverser::reverse(const AudioBuffer& input) {
    // 최적화: reverse iterator로 직접 생성 (복사 1회로 감소)
    const std::vector<float>& inputData = input.getData();
    std::vector<float> data(inputData.rbegin(), inputData.rend());

    // 결과 버퍼 생성
    AudioBuffer result(input.getSampleRate(), input.getChannels());
    result.setData(std::move(data)); // move semantics 사용

    return result;
}
