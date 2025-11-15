#include "AudioReverse.h"
#include <algorithm>

AudioReverse::AudioReverse() {
}

AudioReverse::~AudioReverse() {
}

AudioBuffer AudioReverse::applyReverse(const AudioBuffer& input) {
    // 입력 버퍼를 복사
    AudioBuffer output = input;

    // 데이터 가져오기
    auto& data = output.getData();

    // 역재생: std::reverse 사용
    std::reverse(data.begin(), data.end());

    return output;
}
