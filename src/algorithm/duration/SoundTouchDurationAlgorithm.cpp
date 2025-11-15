#include "SoundTouchDurationAlgorithm.h"

SoundTouchDurationAlgorithm::SoundTouchDurationAlgorithm() {
    soundTouch_.reset(new soundtouch::SoundTouch());
}

SoundTouchDurationAlgorithm::~SoundTouchDurationAlgorithm() {
}

AudioBuffer SoundTouchDurationAlgorithm::stretch(const AudioBuffer& input, float ratio) {
    if (input.getChannels() != 1) {
        return input;
    }

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();

    // SoundTouch 설정
    soundTouch_->setSampleRate(sampleRate);
    soundTouch_->setChannels(1);
    soundTouch_->setTempoChange((ratio - 1.0f) * 100.0f);  // Percentage

    // 입력 데이터 제공
    soundTouch_->putSamples(inputData.data(), inputData.size());
    soundTouch_->flush();

    // 출력 데이터 수신
    std::vector<float> outputData;
    const int BUFFER_SIZE = 2048;
    float buffer[BUFFER_SIZE];

    int receivedSamples = 0;
    do {
        receivedSamples = soundTouch_->receiveSamples(buffer, BUFFER_SIZE);
        if (receivedSamples > 0) {
            outputData.insert(outputData.end(), buffer, buffer + receivedSamples);
        }
    } while (receivedSamples > 0);

    soundTouch_->clear();

    AudioBuffer result(sampleRate, 1);
    result.setData(outputData);
    return result;
}
