#include "RubberBandDurationAlgorithm.h"

using namespace RubberBand;

RubberBandDurationAlgorithm::RubberBandDurationAlgorithm()
    : highQuality_(true) {
}

RubberBandDurationAlgorithm::~RubberBandDurationAlgorithm() {
}

AudioBuffer RubberBandDurationAlgorithm::stretch(const AudioBuffer& input, float ratio) {
    if (input.getChannels() != 1) {
        return input;
    }

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();

    // RubberBand 옵션 설정 (Offline 모드 - 전체 오디오를 미리 가지고 있음)
    RubberBandStretcher::Options options = RubberBandStretcher::OptionProcessOffline;
    if (highQuality_) {
        options |= RubberBandStretcher::OptionEngineFiner;
    }

    RubberBandStretcher stretcher(sampleRate, 1, options);
    stretcher.setTimeRatio(ratio);

    // 입력 데이터 제공 (Offline 모드: study + process)
    const float* inputPtr = inputData.data();
    stretcher.study(&inputPtr, inputData.size(), true);  // study는 offline 모드에서 사용
    stretcher.process(&inputPtr, inputData.size(), true);

    // 출력 데이터 수신
    std::vector<float> outputData;
    int available = stretcher.available();

    if (available > 0) {
        outputData.resize(available);
        float* outputPtr = outputData.data();
        stretcher.retrieve(&outputPtr, available);
    }

    AudioBuffer result(sampleRate, 1);
    result.setData(outputData);
    return result;
}
