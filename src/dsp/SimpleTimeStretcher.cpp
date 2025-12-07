/**
 * SimpleTimeStretcher.cpp
 *
 * WSOLA (Waveform Similarity Overlap-Add) 기반 시간 늘이기/줄이기 알고리즘
 * 대학교 1학년이 이해할 수 있는 수준으로 구현
 *
 * 핵심 개념:
 * 1. 오디오를 작은 조각(segment)으로 나눔
 * 2. 조각들을 적절한 간격으로 배치 (속도에 따라 간격 조정)
 * 3. 조각들을 겹쳐서 더함 (overlap-add)
 * 4. 겹치는 부분에서 가장 유사한 위치를 찾아서 자연스럽게 연결
 */

#include "SimpleTimeStretcher.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SimpleTimeStretcher::SimpleTimeStretcher() {
    // 기본 파라미터 설정 (음악에 적합한 값들)
    sequenceMs = 40;      // 각 조각을 40ms로 설정
    seekWindowMs = 15;    // 15ms 범위 내에서 최적 위치 탐색
    overlapMs = 8;        // 8ms 동안 겹쳐서 합치기
}

AudioBuffer SimpleTimeStretcher::process(const AudioBuffer& input, float ratio, PerformanceChecker* perfChecker) {
    if (ratio <= 0) {
        std::cerr << "[SimpleTimeStretcher] 잘못된 비율: " << ratio << std::endl;
        return input;
    }

    // 비율이 1.0에 가까우면 그냥 원본 반환
    if (std::abs(ratio - 1.0f) < 0.01f) {
        return input;
    }

    const std::vector<float>& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int inputLength = inputData.size();

    // 밀리초를 샘플 수로 변환
    int sequenceSamples = (sequenceMs * sampleRate) / 1000;
    int seekWindowSamples = (seekWindowMs * sampleRate) / 1000;
    int overlapSamples = (overlapMs * sampleRate) / 1000;

    // 출력 버퍼 크기 예측
    int estimatedOutputLength = (int)(inputLength / ratio) + sequenceSamples;
    std::vector<float> outputData;
    outputData.reserve(estimatedOutputLength);

    int inputPos = 0;
    int outputPos = 0;
    bool isFirstSegment = true;

    std::cout << "[SimpleTimeStretcher] 처리 시작 - 비율: " << ratio
              << ", 입력 길이: " << inputLength << " 샘플" << std::endl;

    // 조각별로 처리
    while (inputPos < inputLength - sequenceSamples) {
        int segmentLength = std::min(sequenceSamples, inputLength - inputPos);

        if (isFirstSegment) {
            // 첫 조각은 그냥 복사
            for (int i = 0; i < segmentLength; i++) {
                outputData.push_back(inputData[inputPos + i]);
            }
            outputPos = segmentLength;
            isFirstSegment = false;
        } else {
            // 검색 범위에서 최적 위치 찾기
            int searchStart = inputPos - seekWindowSamples;
            int searchEnd = inputPos + seekWindowSamples;

            searchStart = std::max(0, searchStart);
            searchEnd = std::min(inputLength - overlapSamples, searchEnd);

            // 참조용: 출력의 마지막 부분
            std::vector<float> refSegment(overlapSamples);
            int refStart = outputPos - overlapSamples;
            for (int i = 0; i < overlapSamples; i++) {
                refSegment[i] = outputData[refStart + i];
            }

            // 최적 위치 찾기 (병목 지점 예상)
            if (perfChecker) perfChecker->startFunction("findBestOverlapPosition");
            int bestPos = findBestOverlapPosition(
                inputData,
                searchStart,
                searchEnd - searchStart,
                refSegment,
                overlapSamples
            );
            if (perfChecker) perfChecker->endFunction();

            // 찾은 위치에서 조각 추출하고 겹쳐서 합치기
            if (perfChecker) perfChecker->startFunction("overlapAndAdd");
            overlapAndAdd(outputData, outputPos - overlapSamples,
                         inputData, bestPos, overlapSamples, 1.0f);
            if (perfChecker) perfChecker->endFunction();

            // 나머지 부분 추가
            int remainingLength = sequenceSamples - overlapSamples;
            int copyStart = bestPos + overlapSamples;
            for (int i = 0; i < remainingLength && copyStart + i < inputLength; i++) {
                outputData.push_back(inputData[copyStart + i]);
            }

            outputPos = outputData.size();
        }

        // 다음 입력 위치 계산
        int skip = (int)(sequenceSamples * ratio);
        inputPos += skip;
    }

    // 남은 부분 추가
    while (inputPos < inputLength) {
        outputData.push_back(inputData[inputPos]);
        inputPos++;
    }

    std::cout << "[SimpleTimeStretcher] 처리 완료 - 출력 길이: "
              << outputData.size() << " 샘플" << std::endl;

    AudioBuffer output(sampleRate, 1);
    output.setData(outputData);
    return output;
}

void SimpleTimeStretcher::applyHannWindow(float* buffer, int size) {
    // Hann window: 양 끝을 부드럽게 만들어줌
    for (int i = 0; i < size; i++) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
        buffer[i] *= window;
    }
}

float SimpleTimeStretcher::calculateCorrelation(const float* buf1, const float* buf2, int size) {
    // 상관관계(correlation) 계산
    // 두 신호가 얼마나 비슷한지 측정
    float correlation = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    for (int i = 0; i < size; i++) {
        correlation += buf1[i] * buf2[i];
        norm1 += buf1[i] * buf1[i];
        norm2 += buf2[i] * buf2[i];
    }

    // 정규화
    if (norm1 > 0 && norm2 > 0) {
        correlation /= std::sqrt(norm1 * norm2);
    }

    return correlation;
}

int SimpleTimeStretcher::findBestOverlapPosition(
    const std::vector<float>& input,
    int searchStart,
    int searchLength,
    const std::vector<float>& refSegment,
    int overlapLength)
{
    float bestCorr = -1.0f;
    int bestPos = searchStart;

    // 검색 범위를 돌면서 가장 유사한 위치 찾기
    for (int offset = 0; offset < searchLength - overlapLength; offset++) {
        int currentPos = searchStart + offset;

        if (currentPos + overlapLength > (int)input.size()) {
            break;
        }

        // 유사도 계산
        float corr = calculateCorrelation(
            refSegment.data(),
            &input[currentPos],
            overlapLength
        );

        // 더 유사한 위치 발견시 업데이트
        if (corr > bestCorr) {
            bestCorr = corr;
            bestPos = currentPos;
        }
    }

    return bestPos;
}

void SimpleTimeStretcher::overlapAndAdd(
    std::vector<float>& output,
    int outputPos,
    const std::vector<float>& input,
    int inputPos,
    int length,
    float fadeIn)
{
    // Crossfade: 부드러운 전환
    for (int i = 0; i < length; i++) {
        if (outputPos + i >= (int)output.size()) {
            break;
        }
        if (inputPos + i >= (int)input.size()) {
            break;
        }

        float ratio = (float)i / length;
        float oldSample = output[outputPos + i];
        float newSample = input[inputPos + i];

        output[outputPos + i] = oldSample * (1.0f - ratio) + newSample * ratio;
    }
}
