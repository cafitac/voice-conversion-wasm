#include "ExternalTimeStretchStrategy.h"
#include <vector>
#include <cmath>

ExternalTimeStretchStrategy::ExternalTimeStretchStrategy(bool antiAliasing, bool quickSeek)
    : soundtouch_(new soundtouch::SoundTouch()),
      antiAliasing_(antiAliasing),
      quickSeek_(quickSeek) {

    // Anti-aliasing filter 설정
    soundtouch_->setSetting(SETTING_USE_AA_FILTER, antiAliasing_ ? 1 : 0);

    // Quick seek 설정
    soundtouch_->setSetting(SETTING_USE_QUICKSEEK, quickSeek_ ? 1 : 0);
}

ExternalTimeStretchStrategy::~ExternalTimeStretchStrategy() {
}

AudioBuffer ExternalTimeStretchStrategy::stretch(const AudioBuffer& input, float ratio) {
    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();

    // SoundTouch 초기화
    soundtouch_->setSampleRate(sampleRate);
    soundtouch_->setChannels(channels);

    // Tempo만 설정 (pitch는 유지)
    // ratio > 1.0: 느리게 (tempo 감소)
    // ratio < 1.0: 빠르게 (tempo 증가)
    // tempo = 1 / ratio (ratio가 2.0이면 tempo는 0.5, 즉 절반 속도)
    soundtouch_->setTempo(1.0 / ratio);

    // Pitch는 변경하지 않음 (명시적으로 1.0 설정)
    soundtouch_->setPitch(1.0);

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

void ExternalTimeStretchStrategy::setAntiAliasing(bool enabled) {
    antiAliasing_ = enabled;
    soundtouch_->setSetting(SETTING_USE_AA_FILTER, enabled ? 1 : 0);
}

void ExternalTimeStretchStrategy::setQuickSeek(bool enabled) {
    quickSeek_ = enabled;
    soundtouch_->setSetting(SETTING_USE_QUICKSEEK, enabled ? 1 : 0);
}
