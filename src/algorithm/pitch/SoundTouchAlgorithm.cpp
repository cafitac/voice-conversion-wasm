#include "SoundTouchAlgorithm.h"
#include <cmath>

SoundTouchAlgorithm::SoundTouchAlgorithm(bool antiAliasing, bool quickSeek)
    : antiAliasing_(antiAliasing), quickSeek_(quickSeek) {
    soundTouch_.reset(new soundtouch::SoundTouch());
}

SoundTouchAlgorithm::~SoundTouchAlgorithm() {
}

AudioBuffer SoundTouchAlgorithm::shiftPitch(const AudioBuffer& input, float semitones) {
    if (input.getChannels() != 1) {
        // SoundTouch는 mono만 지원 (이 래퍼에서는)
        return input;
    }

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();

    // SoundTouch 설정
    soundTouch_->setSampleRate(sampleRate);
    soundTouch_->setChannels(1);

    // Pitch shift 설정 (semitones)
    soundTouch_->setPitchSemiTones(semitones);

    // 설정 옵션
    soundTouch_->setSetting(SETTING_USE_AA_FILTER, antiAliasing_ ? 1 : 0);
    soundTouch_->setSetting(SETTING_USE_QUICKSEEK, quickSeek_ ? 1 : 0);

    // 입력 데이터 제공
    soundTouch_->putSamples(inputData.data(), inputData.size());
    soundTouch_->flush();

    // 출력 데이터 수신
    std::vector<float> outputData;
    outputData.reserve(inputData.size() * 2);

    const int BUFFER_SIZE = 2048;
    float buffer[BUFFER_SIZE];

    int receivedSamples = 0;
    do {
        receivedSamples = soundTouch_->receiveSamples(buffer, BUFFER_SIZE);
        if (receivedSamples > 0) {
            outputData.insert(outputData.end(), buffer, buffer + receivedSamples);
        }
    } while (receivedSamples > 0);

    // Clear for next use
    soundTouch_->clear();

    // 결과 AudioBuffer 생성
    AudioBuffer result(sampleRate, 1);
    result.setData(outputData);

    return result;
}
