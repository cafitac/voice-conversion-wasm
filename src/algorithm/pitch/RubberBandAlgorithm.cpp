#include "RubberBandAlgorithm.h"
#include <cmath>

using namespace RubberBand;

RubberBandAlgorithm::RubberBandAlgorithm(bool preserveFormant, bool highQuality)
    : preserveFormant_(preserveFormant), highQuality_(highQuality) {
}

RubberBandAlgorithm::~RubberBandAlgorithm() {
}

AudioBuffer RubberBandAlgorithm::shiftPitch(const AudioBuffer& input, float semitones) {
    if (input.getChannels() != 1) {
        // Mono만 지원
        return input;
    }

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    size_t channels = 1;

    // RubberBand 옵션 설정 (Offline 모드 - 전체 오디오를 미리 가지고 있음)
    RubberBandStretcher::Options options = RubberBandStretcher::OptionProcessOffline;

    if (highQuality_) {
        options |= RubberBandStretcher::OptionEngineFiner;
        options |= RubberBandStretcher::OptionTransientsSmooth;
    }

    if (preserveFormant_) {
        options |= RubberBandStretcher::OptionFormantPreserved;
    }

    // RubberBandStretcher 생성
    RubberBandStretcher stretcher(sampleRate, channels, options);

    // Pitch shift ratio 계산
    double pitchScale = std::pow(2.0, semitones / 12.0);
    stretcher.setPitchScale(pitchScale);

    // Time ratio는 1.0 (duration 유지, pitch만 변경)
    stretcher.setTimeRatio(1.0);

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

    // 결과 AudioBuffer 생성
    AudioBuffer result(sampleRate, 1);
    result.setData(outputData);

    return result;
}
