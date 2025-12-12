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
#include "../audio/BufferPool.h"
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
    // 음수 처리
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

    // 출력 버퍼 크기 예측 (메모리 풀 사용)
    // BufferPool.acquire() 가 이미 resize(estimatedOutputLength) 수행
    int estimatedOutputLength = (int)(inputLength / ratio) + sequenceSamples;
    auto outputData = BufferPool::getInstance().acquire(estimatedOutputLength);

    int inputPos = 0;
    int writePos = 0;
    bool isFirstSegment = true;

    std::cout << "[SimpleTimeStretcher] 처리 시작 - 비율: " << ratio
              << ", 입력 길이: " << inputLength << " 샘플" << std::endl;

    // refSegment를 루프 밖에서 한 번만 생성 (재사용)
    std::vector<float> refSegment(overlapSamples);

    // 조각별로 처리
    // sampleRate 수 => segement 수 만큼 반복 횟수 줄이기
    while (inputPos < inputLength - sequenceSamples) {
        if (isFirstSegment) {
            // 첫 조각: 단순 복사
            appendSegment(outputData, writePos, inputData, inputPos, sequenceSamples);
            isFirstSegment = false;
        } else {
            // 검색 범위 계산
            int searchStart = std::max(0, inputPos - seekWindowSamples);
            int searchEnd = std::min(inputLength - overlapSamples, inputPos + seekWindowSamples);

            // 참조 세그먼트 업데이트 (출력의 마지막 오버랩 부분)
            int refStart = writePos - overlapSamples;
            for (int i = 0; i < overlapSamples; i++) {
                refSegment[i] = outputData[refStart + i];
            }

            // 최적 위치 찾기
            if (perfChecker) perfChecker->startFunction("findBestOverlapPosition");
            int bestPos = findBestOverlapPosition(inputData, searchStart,
                                                  searchEnd - searchStart,
                                                  refSegment, overlapSamples);
            if (perfChecker) perfChecker->endFunction();

            // 오버랩 영역 크로스페이드
            if (perfChecker) perfChecker->startFunction("overlapAndAdd");
            overlapAndAdd(outputData, writePos - overlapSamples,
                         inputData, bestPos, overlapSamples, 1.0f);
            if (perfChecker) perfChecker->endFunction();

            // 나머지 부분 추가
            int remainingLength = sequenceSamples - overlapSamples;
            appendSegment(outputData, writePos, inputData, bestPos + overlapSamples, remainingLength);
        }

        // 다음 입력 위치로 이동
        inputPos += static_cast<int>(sequenceSamples * ratio);
    }

    // 남은 샘플 추가
    int remainingSamples = inputLength - inputPos;
    if (remainingSamples > 0) {
        appendSegment(outputData, writePos, inputData, inputPos, remainingSamples);
    }

    // 실제 사용한 크기로 최종 조정
    outputData.resize(writePos);

    std::cout << "[SimpleTimeStretcher] 처리 완료 - 출력 길이: "
              << writePos << " 샘플" << std::endl;

    AudioBuffer output(sampleRate, 1);
    output.setData(std::move(outputData)); // move semantics
    return output;
}

AudioBuffer SimpleTimeStretcher::processParallel(const AudioBuffer& input, float ratio,
                                                  int numThreads, PerformanceChecker* perfChecker) {
    // 스레드 수 자동 감지
    if (numThreads <= 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;  // fallback
    }

    // 단일 스레드면 일반 process 호출
    if (numThreads == 1) {
        return process(input, ratio, perfChecker);
    }

    int sampleRate = input.getSampleRate();
    const std::vector<float>& inputData = input.getData();
    int inputLength = inputData.size();

    // 세그먼트 크기 계산
    int sequenceSamples = (sequenceMs * sampleRate) / 1000;
    int seekWindowSamples = (seekWindowMs * sampleRate) / 1000;
    int overlapSamples = (overlapMs * sampleRate) / 1000;

    // 출력 버퍼 크기 예측
    int estimatedOutputLength = (int)(inputLength / ratio) + sequenceSamples;
    auto outputData = BufferPool::getInstance().acquire(estimatedOutputLength);

    std::cout << "[SimpleTimeStretcher] 병렬 처리 시작 - 스레드: " << numThreads
              << ", 비율: " << ratio << ", 입력 길이: " << inputLength << " 샘플" << std::endl;

    // 청크 크기 계산 (세그먼트 경계에 맞춤)
    int chunkSamples = (inputLength / numThreads / sequenceSamples) * sequenceSamples;
    if (chunkSamples < sequenceSamples * 2) {
        // 청크가 너무 작으면 단일 스레드로 처리
        return process(input, ratio, perfChecker);
    }

    // 각 스레드의 결과 저장
    std::vector<std::vector<float>> threadResults(numThreads);
    std::vector<std::thread> threads;
    std::mutex outputMutex;

    // 각 스레드에서 청크 처리
    for (int t = 0; t < numThreads; t++) {
        threads.emplace_back([&, t]() {
            int start = t * chunkSamples;
            int end = (t == numThreads - 1) ? inputLength : (t + 1) * chunkSamples;

            if (end > inputLength) end = inputLength;
            if (start >= end) return;

            // 청크 추출
            std::vector<float> chunk(inputData.begin() + start, inputData.begin() + end);

            // AudioBuffer 생성
            AudioBuffer chunkBuffer(sampleRate, 1);
            chunkBuffer.setData(chunk);

            // 처리
            AudioBuffer result = process(chunkBuffer, ratio, nullptr);

            // 결과 저장
            std::lock_guard<std::mutex> lock(outputMutex);
            threadResults[t] = result.getData();
        });
    }

    // 모든 스레드 완료 대기
    for (auto& thread : threads) {
        thread.join();
    }

    // 결과 병합
    int writePos = 0;
    for (int t = 0; t < numThreads; t++) {
        const auto& result = threadResults[t];
        if (result.empty()) continue;

        ensureCapacity(outputData, writePos, result.size());
        for (size_t i = 0; i < result.size(); i++) {
            outputData[writePos++] = result[i];
        }
    }

    outputData.resize(writePos);

    std::cout << "[SimpleTimeStretcher] 병렬 처리 완료 - 출력 길이: "
              << writePos << " 샘플" << std::endl;

    AudioBuffer output(sampleRate, 1);
    output.setData(std::move(outputData));
    return output;
}

float SimpleTimeStretcher::calculateCorrelation(const float* buf1, const float* buf2, int size) {
    // 상관관계(correlation) 계산 - Loop Unrolling 최적화 버전
    // 두 신호가 얼마나 비슷한지 측정
    float correlation = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    // Loop Unrolling: 4개씩 묶어서 처리 (루프 오버헤드 감소 + 컴파일러 자동 벡터화 유도)
    int i = 0;
    int simdSize = size - (size % 4);

    for (; i < simdSize; i += 4) {
        // 4개씩 언롤링 (컴파일러가 SIMD로 자동 벡터화 가능)
        correlation += buf1[i] * buf2[i];
        correlation += buf1[i+1] * buf2[i+1];
        correlation += buf1[i+2] * buf2[i+2];
        correlation += buf1[i+3] * buf2[i+3];

        norm1 += buf1[i] * buf1[i];
        norm1 += buf1[i+1] * buf1[i+1];
        norm1 += buf1[i+2] * buf1[i+2];
        norm1 += buf1[i+3] * buf1[i+3];

        norm2 += buf2[i] * buf2[i];
        norm2 += buf2[i+1] * buf2[i+1];
        norm2 += buf2[i+2] * buf2[i+2];
        norm2 += buf2[i+3] * buf2[i+3];
    }

    // 나머지 처리
    for (; i < size; i++) {
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

    // Early exit threshold: 충분히 좋은 상관관계면 조기 종료
    const float GOOD_ENOUGH_THRESHOLD = 0.95f;

    // Phase 1: Coarse search (2샘플씩 건너뛰며 빠르게 탐색)
    int coarseStep = 2;
    int coarseBestPos = searchStart;
    float coarseBestCorr = -1.0f;

    for (int offset = 0; offset < searchLength - overlapLength; offset += coarseStep) {
        int currentPos = searchStart + offset;

        if (currentPos + overlapLength > (int)input.size()) {
            break;
        }

        float corr = calculateCorrelation(
            refSegment.data(),
            &input[currentPos],
            overlapLength
        );

        if (corr > coarseBestCorr) {
            coarseBestCorr = corr;
            coarseBestPos = currentPos;
        }

        // Early exit: 충분히 좋으면 바로 종료
        if (corr > GOOD_ENOUGH_THRESHOLD) {
            return currentPos;
        }
    }

    // Phase 2: Fine search (coarse 최적 위치 주변을 정밀 탐색)
    int fineSearchStart = std::max(searchStart, coarseBestPos - coarseStep);
    int fineSearchEnd = std::min(searchStart + searchLength - overlapLength,
                                  coarseBestPos + coarseStep + 1);

    bestPos = coarseBestPos;
    bestCorr = coarseBestCorr;

    for (int currentPos = fineSearchStart; currentPos < fineSearchEnd; currentPos++) {
        if (currentPos + overlapLength > (int)input.size()) {
            break;
        }

        float corr = calculateCorrelation(
            refSegment.data(),
            &input[currentPos],
            overlapLength
        );

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

void SimpleTimeStretcher::ensureCapacity(std::vector<float>& buffer, int writePos, int additionalSize) {
    int requiredSize = writePos + additionalSize;
    if (requiredSize > (int)buffer.size()) {
        buffer.resize(requiredSize);
    }
}

void SimpleTimeStretcher::appendSegment(
    std::vector<float>& output,
    int& writePos,
    const std::vector<float>& input,
    int inputPos,
    int length)
{
    ensureCapacity(output, writePos, length);

    int copyLength = std::min(length, (int)input.size() - inputPos);
    for (int i = 0; i < copyLength; i++) {
        output[writePos++] = input[inputPos + i];
    }
}
