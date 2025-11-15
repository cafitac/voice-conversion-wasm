#ifndef PARTIALSEGMENTBENCHMARK_H
#define PARTIALSEGMENTBENCHMARK_H

#include "../audio/AudioBuffer.h"
#include "../effects/ITimeStretchStrategy.h"
#include <string>
#include <vector>
#include <chrono>

/**
 * PartialSegmentBenchmark
 *
 * 부분 구간 시간 늘이기/줄이기 벤치마크
 * - 실제 사용 시나리오: 전체 오디오가 아닌 특정 구간만 처리
 * - 테스트 구간: 500ms, 1s, 2s
 * - 처리 방식: 1.5x (늘이기), 0.75x (줄이기)
 * - 품질 검증: 파형 연속성, 경계 불연속 확인
 */

struct SegmentMetrics {
    std::string algorithmName;
    std::string description;

    // 구간 정보
    float segmentDuration;      // 처리할 구간 길이 (초)
    float segmentStart;         // 구간 시작 시간 (초)
    float segmentEnd;           // 구간 끝 시간 (초)
    float targetRatio;          // 목표 ratio (1.5x = 늘이기, 0.75x = 줄이기)

    // 성능 메트릭
    double processingTimeMs;    // 처리 시간 (ms)
    double realtimeFactor;      // Real-time factor

    // 품질 메트릭
    float originalSegmentDuration;   // 원본 구간 길이
    float outputSegmentDuration;     // 출력 구간 길이
    float actualRatio;              // 실제 ratio
    float durationError;            // 길이 오차 (%)

    // 경계 품질
    float boundaryDiscontinuity;    // 경계 불연속성 (RMS of difference)
    float leftBoundarySmoothl;      // 왼쪽 경계 부드러움
    float rightBoundarySmoothness;  // 오른쪽 경계 부드러움

    // 오디오 출력
    AudioBuffer outputAudio;
};

class PartialSegmentBenchmark {
public:
    PartialSegmentBenchmark();
    ~PartialSegmentBenchmark();

    /**
     * 단일 알고리즘, 단일 구간 벤치마크
     *
     * @param strategy Time stretch strategy
     * @param input 전체 입력 오디오
     * @param segmentStart 구간 시작 시간 (초)
     * @param segmentDuration 구간 길이 (초)
     * @param ratio Time stretch ratio
     * @return 벤치마크 결과
     */
    SegmentMetrics runBenchmark(
        ITimeStretchStrategy* strategy,
        const AudioBuffer& input,
        float segmentStart,
        float segmentDuration,
        float ratio
    );

    /**
     * 모든 알고리즘, 다양한 구간 벤치마크
     *
     * @param input 전체 입력 오디오
     * @param segmentDurations 테스트할 구간 길이 목록 (초)
     * @param ratios 테스트할 ratio 목록
     * @return 모든 벤치마크 결과
     */
    std::vector<SegmentMetrics> runAllBenchmarks(
        const AudioBuffer& input,
        const std::vector<float>& segmentDurations,
        const std::vector<float>& ratios
    );

    /**
     * 벤치마크 결과를 JSON 문자열로 변환
     */
    std::string resultsToJSON(const std::vector<SegmentMetrics>& results);

    /**
     * 벤치마크 결과를 HTML 리포트로 변환
     */
    std::string resultsToHTML(const std::vector<SegmentMetrics>& results);

private:
    /**
     * 부분 구간 처리 (구간 추출 → 처리 → 합성)
     *
     * @param input 전체 오디오
     * @param strategy Time stretch strategy
     * @param startSample 시작 샘플 인덱스
     * @param endSample 끝 샘플 인덱스
     * @param ratio Time stretch ratio
     * @return 처리된 전체 오디오 (구간만 변경됨)
     */
    AudioBuffer processSegment(
        const AudioBuffer& input,
        ITimeStretchStrategy* strategy,
        int startSample,
        int endSample,
        float ratio
    );

    /**
     * 경계 불연속성 측정
     *
     * @param audio 오디오 버퍼
     * @param boundarySample 경계 샘플 인덱스
     * @param windowSize 측정 윈도우 크기
     * @return RMS of difference across boundary
     */
    float measureBoundaryDiscontinuity(
        const AudioBuffer& audio,
        int boundarySample,
        int windowSize
    );

    /**
     * 경계 부드러움 측정 (변화율)
     *
     * @param audio 오디오 버퍼
     * @param boundarySample 경계 샘플 인덱스
     * @param windowSize 측정 윈도우 크기
     * @return 평균 변화율
     */
    float measureBoundarySmoothness(
        const AudioBuffer& audio,
        int boundarySample,
        int windowSize
    );

    /**
     * Crossfade 적용 (경계 부드럽게)
     *
     * @param buffer 오디오 버퍼
     * @param position Crossfade 위치
     * @param fadeLength Crossfade 길이
     */
    void applyCrossfade(
        std::vector<float>& buffer,
        int position,
        int fadeLength
    );

    /**
     * JSON escape 처리
     */
    std::string escapeJSON(const std::string& input);

    /**
     * 타이머 시작
     */
    std::chrono::high_resolution_clock::time_point startTimer();

    /**
     * 타이머 종료 및 경과 시간 반환 (ms)
     */
    double stopTimer(std::chrono::high_resolution_clock::time_point start);
};

#endif // PARTIALSEGMENTBENCHMARK_H
