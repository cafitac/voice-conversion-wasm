#include "ExternalPitchShiftStrategy.h"
#include <vector>
#include <cmath>

ExternalPitchShiftStrategy::ExternalPitchShiftStrategy(bool antiAliasing, bool quickSeek)
    : soundtouch_(new soundtouch::SoundTouch()),
      antiAliasing_(antiAliasing),
      quickSeek_(quickSeek) {

    // Anti-aliasing filter 설정
    soundtouch_->setSetting(SETTING_USE_AA_FILTER, antiAliasing_ ? 1 : 0);

    // Quick seek 설정
    soundtouch_->setSetting(SETTING_USE_QUICKSEEK, quickSeek_ ? 1 : 0);
}

ExternalPitchShiftStrategy::~ExternalPitchShiftStrategy() {
}

AudioBuffer ExternalPitchShiftStrategy::shiftPitch(const AudioBuffer& input, float semitones) {
    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();

    // SoundTouch 초기화
    soundtouch_->setSampleRate(sampleRate);
    soundtouch_->setChannels(channels);
    soundtouch_->setPitchSemiTones(semitones);

    // 입력 데이터 전달
    soundtouch_->putSamples(inputData.data(), inputData.size() / channels);

    // 처리 완료 신호
    soundtouch_->flush();

    // 출력 버퍼 준비
    std::vector<float> outputData;

    // 처리된 샘플 수신
    const int BUFFER_SIZE = 4096;
    std::vector<float> tempBuffer(BUFFER_SIZE * channels);

    int numSamples;
    while ((numSamples = soundtouch_->receiveSamples(tempBuffer.data(), BUFFER_SIZE)) > 0) {
        outputData.insert(outputData.end(),
                         tempBuffer.begin(),
                         tempBuffer.begin() + numSamples * channels);
    }

    // 결과 AudioBuffer 생성
    AudioBuffer result(sampleRate, channels);
    result.setData(outputData);

    // SoundTouch 상태 초기화 (다음 사용을 위해)
    soundtouch_->clear();

    return result;
}

void ExternalPitchShiftStrategy::setAntiAliasing(bool enabled) {
    antiAliasing_ = enabled;
    soundtouch_->setSetting(SETTING_USE_AA_FILTER, enabled ? 1 : 0);
}

void ExternalPitchShiftStrategy::setQuickSeek(bool enabled) {
    quickSeek_ = enabled;
    soundtouch_->setSetting(SETTING_USE_QUICKSEEK, enabled ? 1 : 0);
}
