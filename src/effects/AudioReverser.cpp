#include "AudioReverser.h"
#include <algorithm>

AudioReverser::AudioReverser() {
}

AudioReverser::~AudioReverser() {
}

AudioBuffer AudioReverser::reverse(const AudioBuffer& input) {
    // 입력 버퍼 데이터 복사
    std::vector<float> data = input.getData();

    // 역순으로 뒤집기
    std::reverse(data.begin(), data.end());

    // 결과 버퍼 생성
    AudioBuffer result(input.getSampleRate(), input.getChannels());
    result.setData(data);

    return result;
}
