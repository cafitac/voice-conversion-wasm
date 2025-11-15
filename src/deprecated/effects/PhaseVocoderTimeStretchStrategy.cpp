#include "PhaseVocoderTimeStretchStrategy.h"

PhaseVocoderTimeStretchStrategy::PhaseVocoderTimeStretchStrategy(int fftSize, int hopSize)
    : vocoder_(std::make_unique<PhaseVocoder>(fftSize, hopSize)) {
}

PhaseVocoderTimeStretchStrategy::~PhaseVocoderTimeStretchStrategy() {
}

AudioBuffer PhaseVocoderTimeStretchStrategy::stretch(const AudioBuffer& input, float ratio) {
    if (ratio <= 0.0f) ratio = 1.0f;

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();

    // Phase Vocoder로 time stretch 수행
    auto stretched = vocoder_->timeStretch(inputData, ratio);

    AudioBuffer result(sampleRate, channels);
    result.setData(stretched);

    return result;
}
