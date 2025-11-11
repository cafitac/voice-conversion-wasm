#include "../src/audio/AudioBuffer.h"
#include "../src/utils/WaveFile.h"
#include "../src/effects/ITimeStretchStrategy.h"
#include "../src/effects/FastTimeStretchStrategy.h"
#include "../src/effects/HighQualityTimeStretchStrategy.h"
#include "../src/effects/ExternalTimeStretchStrategy.h"
#include "../src/analysis/PitchAnalyzer.h"
#include "../src/analysis/DurationAnalyzer.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Pitch 측정 (median값 사용) - pitch 보존 확인용
float measureAveragePitch(const AudioBuffer& buffer) {
    PitchAnalyzer analyzer;
    auto pitchPoints = analyzer.analyze(buffer, 0.02f);

    if (pitchPoints.empty()) {
        return 0.0f;
    }

    // Confidence 0.5 이상인 값만 사용
    vector<float> validPitches;
    for (const auto& point : pitchPoints) {
        if (point.confidence > 0.5f && point.frequency > 0.0f) {
            validPitches.push_back(point.frequency);
        }
    }

    if (validPitches.empty()) {
        return 0.0f;
    }

    // Median값 사용 (outlier 제거)
    sort(validPitches.begin(), validPitches.end());
    return validPitches[validPitches.size() / 2];
}

// Duration 측정
float measureDuration(const AudioBuffer& buffer) {
    return static_cast<float>(buffer.getData().size()) /
           static_cast<float>(buffer.getSampleRate() * buffer.getChannels());
}

// 벤치마크 결과 구조체
struct BenchmarkResult {
    string algorithmName;
    double processingTimeMs;
    AudioBuffer outputBuffer;
    float originalPitch;
    float outputPitch;
    float pitchChangePercent;  // pitch 변화 (%)
    float originalDuration;    // seconds
    float outputDuration;      // seconds
    float durationRatio;       // output / input
    float targetRatio;         // 목표 ratio
    float durationError;       // 오차 (%)
};

BenchmarkResult runBenchmark(ITimeStretchStrategy* strategy, const AudioBuffer& input, float targetRatio) {
    BenchmarkResult result;
    result.algorithmName = strategy->getName();
    result.targetRatio = targetRatio;

    // 원본 pitch와 duration 측정
    result.originalPitch = measureAveragePitch(input);
    result.originalDuration = measureDuration(input);

    // Time stretch 수행 (시간 측정)
    auto startTime = high_resolution_clock::now();
    result.outputBuffer = strategy->stretch(input, targetRatio);
    auto endTime = high_resolution_clock::now();

    result.processingTimeMs = duration_cast<microseconds>(endTime - startTime).count() / 1000.0;

    // 출력 pitch와 duration 측정
    result.outputPitch = measureAveragePitch(result.outputBuffer);
    result.outputDuration = measureDuration(result.outputBuffer);

    // Duration ratio 계산
    result.durationRatio = result.outputDuration / result.originalDuration;

    // Duration 오차 계산
    result.durationError = ((result.durationRatio - targetRatio) / targetRatio) * 100.0f;

    // Pitch 변화 계산 (pitch는 보존되어야 함)
    if (result.originalPitch > 0.0f) {
        result.pitchChangePercent = ((result.outputPitch - result.originalPitch) / result.originalPitch) * 100.0f;
    } else {
        result.pitchChangePercent = 0.0f;
    }

    return result;
}

// 결과 출력
void printResults(const vector<BenchmarkResult>& results) {
    cout << "\n========================================" << endl;
    cout << "Time Stretch 알고리즘 비교 결과" << endl;
    cout << "========================================\n" << endl;

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];

        cout << (i + 1) << ". " << r.algorithmName << endl;
        cout << string(50, '-') << endl;

        // 처리 시간
        cout << "처리 시간: " << fixed << setprecision(3)
             << r.processingTimeMs << " ms" << endl;

        // Duration 정확도
        cout << "\nDuration 정확도:" << endl;
        cout << "  - 원본 Duration: " << fixed << setprecision(3)
             << r.originalDuration << " seconds" << endl;
        cout << "  - 출력 Duration: " << fixed << setprecision(3)
             << r.outputDuration << " seconds" << endl;
        cout << "  - 목표 Ratio: " << fixed << setprecision(2)
             << r.targetRatio << "x" << endl;
        cout << "  - 실제 Ratio: " << fixed << setprecision(3)
             << r.durationRatio << "x" << endl;
        cout << "  - Duration 오차: " << fixed << setprecision(2)
             << r.durationError << "%" << endl;

        // Duration 평가
        if (abs(r.durationError) < 1.0f) {
            cout << "  ✅ 평가: Duration 매우 정확" << endl;
        } else if (abs(r.durationError) < 5.0f) {
            cout << "  ✅ 평가: Duration 정확" << endl;
        } else if (abs(r.durationError) < 10.0f) {
            cout << "  ⚠️  평가: Duration 약간 부정확" << endl;
        } else {
            cout << "  ❌ 평가: Duration 부정확" << endl;
        }

        // Pitch 보존 (pitch는 변하지 않아야 함)
        cout << "\nPitch 보존:" << endl;
        cout << "  - 원본 Pitch: " << fixed << setprecision(1)
             << r.originalPitch << " Hz" << endl;
        cout << "  - 출력 Pitch: " << fixed << setprecision(1)
             << r.outputPitch << " Hz" << endl;
        cout << "  - Pitch 변화: " << fixed << setprecision(2)
             << r.pitchChangePercent << "%" << endl;

        // Pitch 평가
        if (abs(r.pitchChangePercent) < 2.0f) {
            cout << "  ✅ 평가: Pitch 완벽 보존" << endl;
        } else if (abs(r.pitchChangePercent) < 5.0f) {
            cout << "  ✅ 평가: Pitch 잘 보존됨" << endl;
        } else if (abs(r.pitchChangePercent) < 10.0f) {
            cout << "  ⚠️  평가: Pitch 약간 변화" << endl;
        } else {
            cout << "  ❌ 평가: Pitch 크게 변화 (버그 가능성)" << endl;
        }

        cout << endl;
    }

    // 종합 비교
    cout << "\n========================================" << endl;
    cout << "종합 비교" << endl;
    cout << "========================================\n" << endl;

    // 처리 속도 순위
    cout << "처리 속도 순위:" << endl;
    auto speedResults = results;
    sort(speedResults.begin(), speedResults.end(),
         [](const BenchmarkResult& a, const BenchmarkResult& b) {
             return a.processingTimeMs < b.processingTimeMs;
         });

    for (size_t i = 0; i < speedResults.size(); ++i) {
        cout << "  " << (i + 1) << ". " << speedResults[i].algorithmName
             << " - " << fixed << setprecision(3)
             << speedResults[i].processingTimeMs << " ms" << endl;
    }

    // Duration 정확도 순위
    cout << "\nDuration 정확도 순위:" << endl;
    auto accuracyResults = results;
    sort(accuracyResults.begin(), accuracyResults.end(),
         [](const BenchmarkResult& a, const BenchmarkResult& b) {
             return abs(a.durationError) < abs(b.durationError);
         });

    for (size_t i = 0; i < accuracyResults.size(); ++i) {
        cout << "  " << (i + 1) << ". " << accuracyResults[i].algorithmName
             << " - 오차 " << fixed << setprecision(2)
             << abs(accuracyResults[i].durationError) << "%" << endl;
    }

    // 추천 전략
    cout << "\n추천 전략:" << endl;
    cout << "  🥇 속도 최우선: " << speedResults[0].algorithmName << endl;
    cout << "  🥇 정확도 최우선: " << accuracyResults[0].algorithmName << endl;
    cout << "  🥇 균형: ExternalTimeStretch (SoundTouch) 추천" << endl;
}

int main(int argc, char* argv[]) {
    cout << "Time Stretch 벤치마크 테스트\n" << endl;

    // 입력 파일 확인
    string inputFile = (argc > 1) ? argv[1] : "original.wav";
    float targetRatio = (argc > 2) ? stof(argv[2]) : 1.5f;  // 기본값: 1.5x (느리게)

    cout << "입력 파일: " << inputFile << endl;
    cout << "목표 Time Stretch Ratio: " << fixed << setprecision(2)
         << targetRatio << "x" << endl;
    cout << "(ratio > 1.0: 느리게, ratio < 1.0: 빠르게)\n" << endl;

    // WAV 파일 로드
    WaveFile waveFile;
    AudioBuffer input = waveFile.load(inputFile);
    if (input.getData().empty()) {
        cerr << "Error: 파일을 읽을 수 없습니다: " << inputFile << endl;
        return 1;
    }

    cout << "오디오 정보:" << endl;
    cout << "  - 샘플 레이트: " << input.getSampleRate() << " Hz" << endl;
    cout << "  - 오디오 길이: " << fixed << setprecision(2)
         << measureDuration(input) << " seconds" << endl;
    cout << "  - 원본 Pitch: " << fixed << setprecision(1)
         << measureAveragePitch(input) << " Hz (median)\n" << endl;

    // 벤치마크 실행
    vector<BenchmarkResult> results;

    // 1. Fast (Frame Repeat/Skip)
    cout << "Testing FastTimeStretch..." << endl;
    FastTimeStretchStrategy fastStrategy;
    results.push_back(runBenchmark(&fastStrategy, input, targetRatio));

    // 2. High Quality (WSOLA)
    cout << "Testing HighQualityTimeStretch (WSOLA)..." << endl;
    HighQualityTimeStretchStrategy highQualityStrategy(1024, 256);
    results.push_back(runBenchmark(&highQualityStrategy, input, targetRatio));

    // 3. External (SoundTouch)
    cout << "Testing ExternalTimeStretch (SoundTouch)..." << endl;
    ExternalTimeStretchStrategy externalStrategy(true, false);
    results.push_back(runBenchmark(&externalStrategy, input, targetRatio));

    cout << endl;

    // 결과 출력
    printResults(results);

    // 결과 파일 저장
    cout << "\n결과 파일 저장 중..." << endl;
    WaveFile outputWaveFile;
    for (const auto& r : results) {
        string outputFile = "output_timestretch_" +
                           string(r.algorithmName.find("Fast") != string::npos ? "fast" :
                                  r.algorithmName.find("HighQuality") != string::npos ? "high" :
                                  "external") + ".wav";
        outputWaveFile.save(outputFile, r.outputBuffer);
        cout << "  - " << outputFile << endl;
    }

    cout << "\n✓ 벤치마크 완료!" << endl;
    return 0;
}
