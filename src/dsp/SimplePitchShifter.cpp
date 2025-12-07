/**
 * SimplePitchShifter.cpp
 *
 * 피치 변경 알고리즘 (Time Stretch + Resampling 조합)
 * 대학교 1학년이 이해할 수 있는 수준으로 구현
 *
 * 핵심 개념:
 * 1. Time Stretch로 오디오 길이를 변경 (피치도 같이 변함)
 * 2. Resampling으로 원래 길이로 되돌림 (피치만 변경됨)
 *
 * 예시:
 * - 피치를 높이려면: 먼저 느리게 만들고(stretch), 빠르게 재생(resample)
 * - 피치를 낮추려면: 먼저 빠르게 만들고(compress), 느리게 재생(resample)
 */

#include "SimplePitchShifter.h"
#include <cmath>
#include <iostream>

SimplePitchShifter::SimplePitchShifter() {
    // 생성자
}

AudioBuffer SimplePitchShifter::process(const AudioBuffer& input, float semitones, PerformanceChecker* perfChecker) {
    // 변화가 거의 없으면 원본 반환
    if (std::abs(semitones) < 0.01f) {
        return input;
    }

    std::cout << "[SimplePitchShifter] 처리 시작 - 반음: " << semitones << std::endl;

    // Step 1: 반음을 비율로 변환
    if (perfChecker) perfChecker->startFunction("semitonesToRatio");
    float pitchRatio = semitonesToRatio(semitones);
    if (perfChecker) perfChecker->endFunction();

    std::cout << "[SimplePitchShifter] 피치 비율: " << pitchRatio << std::endl;

    // Step 2: Time Stretch 적용
    // 피치를 높이려면: 먼저 느리게 (1/pitchRatio)
    // 피치를 낮추려면: 먼저 빠르게 (1/pitchRatio)
    float stretchRatio = 1.0f / pitchRatio;
    if (perfChecker) perfChecker->startFunction("timeStretcher.process");
    AudioBuffer stretched = timeStretcher.process(input, stretchRatio, perfChecker);
    if (perfChecker) perfChecker->endFunction();

    std::cout << "[SimplePitchShifter] Time stretch 완료 - 비율: " << stretchRatio << std::endl;

    // Step 3: Resampling으로 원래 길이로 복원
    // 이 과정에서 피치만 변경됨
    if (perfChecker) perfChecker->startFunction("resample");
    AudioBuffer result = resample(stretched, pitchRatio);
    if (perfChecker) perfChecker->endFunction();

    std::cout << "[SimplePitchShifter] 리샘플링 완료 - 최종 길이: "
              << result.getLength() << " 샘플" << std::endl;

    return result;
}

float SimplePitchShifter::semitonesToRatio(float semitones) {
    // 반음을 주파수 비율로 변환
    // 공식: ratio = 2^(semitones/12)
    //
    // 예시:
    // +12 반음 (한 옥타브 위) = 2^(12/12) = 2.0 (주파수 2배)
    // +7 반음 (완전5도 위) = 2^(7/12) = 1.498 (약 1.5배)
    // -12 반음 (한 옥타브 아래) = 2^(-12/12) = 0.5 (주파수 절반)

    return std::pow(2.0f, semitones / 12.0f);
}

AudioBuffer SimplePitchShifter::resample(const AudioBuffer& input, float ratio) {
    const std::vector<float>& inputData = input.getData();
    int inputLength = inputData.size();
    int sampleRate = input.getSampleRate();

    // 출력 길이 계산
    // ratio > 1.0: 더 짧아짐 (빠르게 재생)
    // ratio < 1.0: 더 길어짐 (느리게 재생)
    int outputLength = (int)(inputLength / ratio);

    std::vector<float> outputData(outputLength);

    std::cout << "[SimplePitchShifter] 리샘플링 - 입력: " << inputLength
              << " -> 출력: " << outputLength << " 샘플" << std::endl;

    // SIMD 최적화: 4개씩 묶어서 처리
    int i = 0;
    int simdSize = outputLength - 3;

    for (; i < simdSize; i += 4) {
        // 4개의 출력 샘플을 한 번에 계산 (컴파일러 SIMD 힌트)
        float inputPos0 = i * ratio;
        float inputPos1 = (i + 1) * ratio;
        float inputPos2 = (i + 2) * ratio;
        float inputPos3 = (i + 3) * ratio;

        int index0 = (int)inputPos0;
        int index1 = (int)inputPos1;
        int index2 = (int)inputPos2;
        int index3 = (int)inputPos3;

        float frac0 = inputPos0 - index0;
        float frac1 = inputPos1 - index1;
        float frac2 = inputPos2 - index2;
        float frac3 = inputPos3 - index3;

        // 범위 체크 및 보간
        if (index0 < inputLength - 1) {
            outputData[i] = inputData[index0] * (1.0f - frac0) + inputData[index0 + 1] * frac0;
        } else {
            outputData[i] = inputData[inputLength - 1];
        }

        if (index1 < inputLength - 1) {
            outputData[i + 1] = inputData[index1] * (1.0f - frac1) + inputData[index1 + 1] * frac1;
        } else {
            outputData[i + 1] = inputData[inputLength - 1];
        }

        if (index2 < inputLength - 1) {
            outputData[i + 2] = inputData[index2] * (1.0f - frac2) + inputData[index2 + 1] * frac2;
        } else {
            outputData[i + 2] = inputData[inputLength - 1];
        }

        if (index3 < inputLength - 1) {
            outputData[i + 3] = inputData[index3] * (1.0f - frac3) + inputData[index3 + 1] * frac3;
        } else {
            outputData[i + 3] = inputData[inputLength - 1];
        }
    }

    // 나머지 처리
    for (; i < outputLength; i++) {
        float inputPos = i * ratio;
        int index = (int)inputPos;
        float fraction = inputPos - index;

        if (index >= inputLength - 1) {
            outputData[i] = inputData[inputLength - 1];
        } else {
            outputData[i] = linearInterpolate(inputData[index], inputData[index + 1], fraction);
        }
    }

    // 결과 AudioBuffer 생성
    AudioBuffer output(sampleRate, 1);
    output.setData(std::move(outputData)); // move semantics
    return output;
}

float SimplePitchShifter::linearInterpolate(float sample1, float sample2, float fraction) {
    // 선형 보간 (Linear Interpolation)
    //
    // 두 점 사이의 직선상에 있는 값을 계산
    // 공식: result = sample1 * (1 - fraction) + sample2 * fraction
    //
    // 예시:
    // sample1 = 1.0, sample2 = 3.0, fraction = 0.5
    // result = 1.0 * 0.5 + 3.0 * 0.5 = 2.0 (정확히 중간값)
    //
    // sample1 = 1.0, sample2 = 3.0, fraction = 0.25
    // result = 1.0 * 0.75 + 3.0 * 0.25 = 1.5 (1에 가까움)

    return sample1 * (1.0f - fraction) + sample2 * fraction;
}
