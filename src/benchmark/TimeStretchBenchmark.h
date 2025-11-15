#ifndef TIMESTRETCHBENCHMARK_H
#define TIMESTRETCHBENCHMARK_H

#include "../audio/AudioBuffer.h"
#include "../effects/ITimeStretchStrategy.h"
#include <string>
#include <vector>
#include <chrono>

/**
 * TimeStretchBenchmark
 *
 * Time Stretch 알고리즘 벤치마크 시스템
 * - 5가지 알고리즘 성능 비교 (Fast, WSOLA, SoundTouch, Phase Vocoder, RubberBand)
 * - 성능 메트릭: 처리 시간, 처리량, Real-time factor
 * - 품질 메트릭: SNR, RMS Error
 */

struct BenchmarkMetrics {
    std::string algorithmName;
    float ratio;                    // Time stretch ratio

    // 성능 메트릭
    double processingTimeMs;        // 처리 시간 (밀리초)
    double throughputSamplesPerSec; // 처리량 (samples/sec)
    double realtimeFactor;          // Real-time factor (처리 시간 / 오디오 길이)

    // 품질 메트릭
    double snr;                     // Signal-to-Noise Ratio (dB)
    double rmsError;                // RMS Error
    float originalPitch;            // 원본 pitch (Hz)
    float outputPitch;              // 출력 pitch (Hz)
    float pitchChangePercent;       // Pitch 변화 (%)
    float originalDuration;         // 원본 duration (seconds)
    float outputDuration;           // 출력 duration (seconds)
    float durationRatio;            // 실제 duration ratio
    float durationError;            // Duration 오차 (%)

    // 출력 오디오
    AudioBuffer outputAudio;
};

class TimeStretchBenchmark {
public:
    TimeStretchBenchmark();
    ~TimeStretchBenchmark();

    /**
     * 단일 알고리즘 벤치마크 실행
     * @param strategy Time stretch strategy
     * @param input 입력 오디오
     * @param ratio Time stretch ratio
     * @return 벤치마크 결과
     */
    BenchmarkMetrics runBenchmark(
        ITimeStretchStrategy* strategy,
        const AudioBuffer& input,
        float ratio
    );

    /**
     * 모든 알고리즘 벤치마크 실행
     * @param input 입력 오디오
     * @param ratios 테스트할 ratio 목록
     * @return 모든 벤치마크 결과
     */
    std::vector<BenchmarkMetrics> runAllBenchmarks(
        const AudioBuffer& input,
        const std::vector<float>& ratios
    );

    /**
     * 벤치마크 결과를 JSON 문자열로 변환
     */
    std::string resultsToJSON(const std::vector<BenchmarkMetrics>& results);

    /**
     * 벤치마크 결과를 HTML 리포트로 변환
     */
    std::string resultsToHTML(const std::vector<BenchmarkMetrics>& results);

private:
    /**
     * Signal-to-Noise Ratio 계산
     * @param original 원본 신호
     * @param processed 처리된 신호
     * @return SNR (dB)
     */
    double calculateSNR(const AudioBuffer& original, const AudioBuffer& processed);

    /**
     * RMS Error 계산
     * @param original 원본 신호
     * @param processed 처리된 신호
     * @return RMS Error
     */
    double calculateRMSError(const AudioBuffer& original, const AudioBuffer& processed);

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

#endif // TIMESTRETCHBENCHMARK_H
