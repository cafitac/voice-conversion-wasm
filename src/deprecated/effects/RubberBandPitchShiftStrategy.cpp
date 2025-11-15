#include "RubberBandPitchShiftStrategy.h"
#include <cmath>

extern "C" {
#include "../external/rubberband/rubberband/rubberband-c.h"
}

RubberBandPitchShiftStrategy::RubberBandPitchShiftStrategy(bool preserveFormant, bool highQuality)
    : m_preserveFormant(preserveFormant), m_highQuality(highQuality) {
}

RubberBandPitchShiftStrategy::~RubberBandPitchShiftStrategy() {
}

AudioBuffer RubberBandPitchShiftStrategy::shiftPitch(const AudioBuffer& input, float semitones) {
    if (input.getData().empty()) {
        return AudioBuffer(input.getSampleRate(), input.getChannels());
    }

    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();
    const auto& inputData = input.getData();
    int totalSamples = inputData.size() / channels;

    // Pitch scale 계산 (semitones -> ratio)
    double pitchScale = std::pow(2.0, semitones / 12.0);
    double timeRatio = 1.0;  // 시간은 유지

    // RubberBand 옵션 설정
    RubberBandOptions options = RubberBandOptionProcessOffline | RubberBandOptionEngineFiner;

    // 포먼트 보존 옵션
    if (m_preserveFormant) {
        options |= RubberBandOptionFormantPreserved;
    }

    // 품질 옵션
    if (m_highQuality) {
        options |= RubberBandOptionPitchHighQuality;
    } else {
        options |= RubberBandOptionPitchHighSpeed;
    }

    // 트랜지언트 설정 (피치 변조는 Mixed가 적합)
    options |= RubberBandOptionTransientsMixed;

    // RubberBand 생성
    RubberBandState rubberband = rubberband_new(
        sampleRate,
        channels,
        options,
        timeRatio,
        pitchScale
    );

    if (!rubberband) {
        return AudioBuffer(sampleRate, channels);
    }

    // 예상 입력 길이 설정
    rubberband_set_expected_input_duration(rubberband, totalSamples);

    // 채널별 데이터 준비
    std::vector<float*> channelBuffers(channels);
    std::vector<std::vector<float>> channelData(channels);

    for (int c = 0; c < channels; ++c) {
        channelData[c].resize(totalSamples);
        for (int i = 0; i < totalSamples; ++i) {
            channelData[c][i] = inputData[i * channels + c];
        }
        channelBuffers[c] = channelData[c].data();
    }

    // 데이터 처리
    rubberband_study(rubberband, channelBuffers.data(), totalSamples, 1);
    rubberband_process(rubberband, channelBuffers.data(), totalSamples, 1);

    // 출력 데이터 가져오기
    int available = rubberband_available(rubberband);
    std::vector<float*> outputChannelBuffers(channels);
    std::vector<std::vector<float>> outputChannelData(channels);

    for (int c = 0; c < channels; ++c) {
        outputChannelData[c].resize(available);
        outputChannelBuffers[c] = outputChannelData[c].data();
    }

    int retrieved = rubberband_retrieve(rubberband, outputChannelBuffers.data(), available);

    // 인터리브된 출력 생성
    std::vector<float> output;
    output.reserve(retrieved * channels);

    for (int i = 0; i < retrieved; ++i) {
        for (int c = 0; c < channels; ++c) {
            output.push_back(outputChannelData[c][i]);
        }
    }

    // 정리
    rubberband_delete(rubberband);

    AudioBuffer result(sampleRate, channels);
    result.setData(output);
    return result;
}

const char* RubberBandPitchShiftStrategy::getName() const {
    return "RubberBand Pitch Shift";
}
