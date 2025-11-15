#ifndef COMBINEDBENCHMARK_H
#define COMBINEDBENCHMARK_H

#include "../audio/AudioBuffer.h"
#include <string>
#include <vector>
#include <chrono>

/**
 * CombinedBenchmark
 *
 * Pitch + Duration 결합 처리 벤치마크 시스템
 * - 3가지 방식 비교:
 *   1. Sequential: Pitch then TimeStretch
 *   2. Sequential: TimeStretch then Pitch
 *   3. Direct: SoundTouch Combined
 * - 성능 메트릭: 처리 시간
 * - 품질 메트릭: Pitch 정확도, Duration 정확도
 */

struct CombinedMetrics {
    std::string methodName;
    float targetPitchSemitones;    // 목표 pitch shift
    float targetDurationRatio;     // 목표 duration ratio

    // 성능 메트릭
    double processingTimeMs;       // 처리 시간 (밀리초)

    // Pitch 품질 메트릭
    float originalPitch;           // 원본 pitch (Hz)
    float outputPitch;             // 출력 pitch (Hz)
    float actualPitchSemitones;    // 실제 pitch shift (semitones)
    float pitchError;              // Pitch 오차 (semitones)

    // Duration 품질 메트릭
    float originalDuration;        // 원본 duration (seconds)
    float outputDuration;          // 출력 duration (seconds)
    float actualDurationRatio;     // 실제 duration ratio
    float durationError;           // Duration 오차 (%)

    // 출력 오디오
    AudioBuffer outputAudio;
};

class CombinedBenchmark {
public:
    CombinedBenchmark();
    ~CombinedBenchmark();

    /**
     * Sequential: Pitch then TimeStretch 벤치마크
     * @param input 입력 오디오
     * @param semitones Pitch shift amount
     * @param durationRatio Duration ratio
     * @return 벤치마크 결과
     */
    CombinedMetrics runPitchThenStretch(
        const AudioBuffer& input,
        float semitones,
        float durationRatio
    );

    /**
     * Sequential: TimeStretch then Pitch 벤치마크
     * @param input 입력 오디오
     * @param semitones Pitch shift amount
     * @param durationRatio Duration ratio
     * @return 벤치마크 결과
     */
    CombinedMetrics runStretchThenPitch(
        const AudioBuffer& input,
        float semitones,
        float durationRatio
    );

    /**
     * Direct: SoundTouch Combined 벤치마크
     * @param input 입력 오디오
     * @param semitones Pitch shift amount
     * @param durationRatio Duration ratio
     * @return 벤치마크 결과
     */
    CombinedMetrics runSoundTouchDirect(
        const AudioBuffer& input,
        float semitones,
        float durationRatio
    );

    /**
     * Sequential: Phase Vocoder (Pitch + TimeStretch)
     * @param input 입력 오디오
     * @param semitones Pitch shift amount
     * @param durationRatio Duration ratio
     * @return 벤치마크 결과
     */
    CombinedMetrics runPhaseVocoderCombined(
        const AudioBuffer& input,
        float semitones,
        float durationRatio
    );

    /**
     * Direct: RubberBand Combined 벤치마크
     * @param input 입력 오디오
     * @param semitones Pitch shift amount
     * @param durationRatio Duration ratio
     * @return 벤치마크 결과
     */
    CombinedMetrics runRubberBandDirect(
        const AudioBuffer& input,
        float semitones,
        float durationRatio
    );

    /**
     * 모든 방식 벤치마크 실행
     * @param input 입력 오디오
     * @param semitones Pitch shift amount
     * @param durationRatio Duration ratio
     * @return 모든 벤치마크 결과
     */
    std::vector<CombinedMetrics> runAllBenchmarks(
        const AudioBuffer& input,
        float semitones,
        float durationRatio
    );

    /**
     * 벤치마크 결과를 JSON 문자열로 변환
     */
    std::string resultsToJSON(const std::vector<CombinedMetrics>& results);

    /**
     * 벤치마크 결과를 HTML 리포트로 변환
     */
    std::string resultsToHTML(const std::vector<CombinedMetrics>& results);

private:
    /**
     * Pitch 측정 (median)
     */
    float measureAveragePitch(const AudioBuffer& buffer);

    /**
     * Duration 측정
     */
    float measureDuration(const AudioBuffer& buffer);

    /**
     * Semitones 계산
     */
    float hertzToSemitones(float originalHz, float targetHz);

    /**
     * 처리 시간 측정
     */
    std::chrono::high_resolution_clock::time_point startTimer();
    double stopTimer(std::chrono::high_resolution_clock::time_point start);

    /**
     * JSON escape 처리
     */
    std::string escapeJSON(const std::string& input);
};

#endif // COMBINEDBENCHMARK_H
