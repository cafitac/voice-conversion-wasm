#include "RubberBandTimeStretchStrategy.h"
#include "rubberband/rubberband-c.h"
#include <vector>
#include <cstring>

RubberBandTimeStretchStrategy::RubberBandTimeStretchStrategy() {
}

RubberBandTimeStretchStrategy::~RubberBandTimeStretchStrategy() {
}

AudioBuffer RubberBandTimeStretchStrategy::stretch(const AudioBuffer& input, float ratio) {
    if (ratio <= 0.0f) ratio = 1.0f;

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();
    int inputLength = inputData.size();

    // RubberBand 옵션 설정
    // - ProcessOffline: 오프라인 처리 (최고 품질)
    // - EngineFiner: Finer 엔진 (높은 품질)
    // - TransientsMixed: Mixed transient handling (균형잡힌 설정)
    RubberBandOptions options =
        RubberBandOptionProcessOffline |
        RubberBandOptionEngineFiner |
        RubberBandOptionTransientsMixed;

    // RubberBand 인스턴스 생성
    // timeRatio: 1.5 = 느려짐 (50% 더 긴), 0.5 = 빨라짐 (50% 짧아짐)
    // pitchScale: 1.0 = 피치 변화 없음
    RubberBandState rubberband = rubberband_new(
        sampleRate,
        channels,
        options,
        ratio,  // time ratio
        1.0     // pitch scale (no pitch change)
    );

    if (!rubberband) {
        // RubberBand 생성 실패 시 원본 반환
        return input;
    }

    // 예상 입력 길이 설정
    rubberband_set_expected_input_duration(rubberband, inputLength);

    // 입력 데이터를 채널별로 분리
    std::vector<float*> inputChannels(channels);
    std::vector<std::vector<float>> channelData(channels);

    for (int ch = 0; ch < channels; ++ch) {
        channelData[ch].resize(inputLength);
        for (int i = 0; i < inputLength; ++i) {
            channelData[ch][i] = inputData[i * channels + ch];
        }
        inputChannels[ch] = channelData[ch].data();
    }

    // 입력 데이터 처리 (final = 1로 마지막임을 알림)
    rubberband_process(rubberband, inputChannels.data(), inputLength, 1);

    // 출력 데이터 가져오기
    int outputLength = static_cast<int>(inputLength * ratio);
    std::vector<float*> outputChannels(channels);
    std::vector<std::vector<float>> outputChannelData(channels);

    for (int ch = 0; ch < channels; ++ch) {
        outputChannelData[ch].resize(outputLength);
        outputChannels[ch] = outputChannelData[ch].data();
    }

    // 사용 가능한 모든 샘플 가져오기
    int available = rubberband_available(rubberband);
    if (available < 0) available = outputLength;

    int retrieved = rubberband_retrieve(rubberband, outputChannels.data(), available);

    // 채널별 데이터를 인터리브 형식으로 변환
    std::vector<float> outputData;
    outputData.reserve(retrieved * channels);

    for (int i = 0; i < retrieved; ++i) {
        for (int ch = 0; ch < channels; ++ch) {
            outputData.push_back(outputChannelData[ch][i]);
        }
    }

    // RubberBand 정리
    rubberband_delete(rubberband);

    // 결과 반환
    AudioBuffer result(sampleRate, channels);
    result.setData(outputData);

    return result;
}
