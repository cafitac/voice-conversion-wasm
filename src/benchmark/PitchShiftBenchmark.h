#ifndef PITCHSHIFTBENCHMARK_H
#define PITCHSHIFTBENCHMARK_H

#include "../audio/AudioBuffer.h"
#include "../effects/IPitchShiftStrategy.h"
#include <string>
#include <vector>
#include <chrono>

/**
 * PitchShiftBenchmark
 *
 * Pitch Shift 알고리즘 벤치마크 시스템
 * - 3가지 알고리즘 성능 비교 (Fast, Phase Vocoder, SoundTouch)
 * - 성능 메트릭: 처리 시간, 처리량, Real-time factor
 * - 품질 메트릭: Pitch 정확도, RMS Error, Duration 변화
 */

struct PitchShiftMetrics {
    std::string algorithmName;
    float semitones;                // 목표 pitch shift (semitones)

    // 성능 메트릭
    double processingTimeMs;        // 처리 시간 (밀리초)
    double throughputSamplesPerSec; // 처리량 (samples/sec)
    double realtimeFactor;          // Real-time factor (처리 시간 / 오디오 길이)

    // 품질 메트릭
    float originalPitch;            // 원본 pitch (Hz)
    float outputPitch;              // 출력 pitch (Hz)
    float actualPitchSemitones;     // 실제 pitch shift (semitones)
    float pitchError;               // Pitch 오차 (semitones)
    float durationRatio;            // Duration ratio (output / input)
    double rmsError;                // RMS Error

    // 출력 오디오
    AudioBuffer outputAudio;
};

class PitchShiftBenchmark {
public:
    PitchShiftBenchmark();
    ~PitchShiftBenchmark();

    /**
     * 단일 알고리즘 벤치마크 실행
     * @param strategy Pitch shift strategy
     * @param input 입력 오디오
     * @param semitones Pitch shift amount (semitones)
     * @return 벤치마크 결과
     */
    PitchShiftMetrics runBenchmark(
        IPitchShiftStrategy* strategy,
        const AudioBuffer& input,
        float semitones
    );

    /**
     * 모든 알고리즘 벤치마크 실행
     * @param input 입력 오디오
     * @param semitonesValues 테스트할 semitones 목록
     * @return 모든 벤치마크 결과
     */
    std::vector<PitchShiftMetrics> runAllBenchmarks(
        const AudioBuffer& input,
        const std::vector<float>& semitonesValues
    );

    /**
     * 벤치마크 결과를 JSON 문자열로 변환
     */
    std::string resultsToJSON(const std::vector<PitchShiftMetrics>& results);

    /**
     * 벤치마크 결과를 HTML 리포트로 변환
     */
    std::string resultsToHTML(const std::vector<PitchShiftMetrics>& results);

private:
    /**
     * Pitch 측정 (median)
     */
    float measureAveragePitch(const AudioBuffer& buffer);

    /**
     * RMS Error 계산
     */
    double calculateRMSError(const AudioBuffer& original, const AudioBuffer& processed);

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

#endif // PITCHSHIFTBENCHMARK_H
