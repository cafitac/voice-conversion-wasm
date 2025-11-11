#include "../src/audio/AudioBuffer.h"
#include "../src/utils/WaveFile.h"
#include "../src/effects/IPitchShiftStrategy.h"
#include "../src/effects/FastPitchShiftStrategy.h"
#include "../src/effects/HighQualityPitchShiftStrategy.h"
#include "../src/effects/ExternalPitchShiftStrategy.h"
#include "../src/analysis/PitchAnalyzer.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Pitch 측정 (median값 사용)
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

// RMS 오차 계산
float calculateRMSError(const vector<float>& original, const vector<float>& processed) {
    if (original.size() != processed.size()) {
        cerr << "Warning: Size mismatch in RMS calculation" << endl;
        size_t minSize = min(original.size(), processed.size());

        float sumSquaredError = 0.0f;
        for (size_t i = 0; i < minSize; ++i) {
            float error = original[i] - processed[i];
            sumSquaredError += error * error;
        }
        return sqrt(sumSquaredError / minSize);
    }

    float sumSquaredError = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float error = original[i] - processed[i];
        sumSquaredError += error * error;
    }
    return sqrt(sumSquaredError / original.size());
}

// 벤치마크 실행
struct BenchmarkResult {
    string algorithmName;
    double processingTimeMs;
    float rmsError;
    AudioBuffer outputBuffer;
    float originalPitch;
    float outputPitch;
    float pitchShiftActual;  // semitones
    float durationRatio;  // output / input
};

BenchmarkResult runBenchmark(IPitchShiftStrategy* strategy, const AudioBuffer& input, float semitones) {
    BenchmarkResult result;
    result.algorithmName = strategy->getName();

    // 원본 pitch 측정
    result.originalPitch = measureAveragePitch(input);

    // 처리 시간 측정
    auto start = high_resolution_clock::now();
    result.outputBuffer = strategy->shiftPitch(input, semitones);
    auto end = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(end - start);
    result.processingTimeMs = duration.count() / 1000.0;

    // 출력 pitch 측정
    result.outputPitch = measureAveragePitch(result.outputBuffer);

    // 실제 pitch shift 계산 (semitones)
    if (result.originalPitch > 0.0f && result.outputPitch > 0.0f) {
        result.pitchShiftActual = 12.0f * log2(result.outputPitch / result.originalPitch);
    } else {
        result.pitchShiftActual = 0.0f;
    }

    // Duration 비율 계산
    result.durationRatio = result.outputBuffer.getDuration() / input.getDuration();

    // RMS 오차 계산 (원본과 비교)
    result.rmsError = calculateRMSError(input.getData(), result.outputBuffer.getData());

    return result;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.wav>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    float semitones = 3.0f;  // +3 semitones

    cout << "===========================================================" << endl;
    cout << "          Pitch Shifting Benchmark Report" << endl;
    cout << "===========================================================" << endl;
    cout << endl;

    // WAV 파일 로드
    WaveFile wavFile;
    AudioBuffer input = wavFile.load(inputFile);

    cout << "Input: " << inputFile << endl;
    cout << "Sample Rate: " << input.getSampleRate() << " Hz" << endl;
    cout << "Duration: " << input.getDuration() << " seconds" << endl;
    cout << "Pitch Shift: +" << semitones << " semitones" << endl;
    cout << endl;

    // 3가지 알고리즘 테스트
    vector<BenchmarkResult> results;

    // 1. Fast (Simple Resampling)
    cout << "Testing Fast (Simple Resampling)..." << endl;
    FastPitchShiftStrategy fastStrategy;
    results.push_back(runBenchmark(&fastStrategy, input, semitones));

    // 2. High Quality (Phase Vocoder - 직접 구현)
    cout << "Testing High Quality (Phase Vocoder)..." << endl;
    HighQualityPitchShiftStrategy highQualityStrategy(1024, 256);
    results.push_back(runBenchmark(&highQualityStrategy, input, semitones));

    // 3. External (SoundTouch)
    cout << "Testing External (SoundTouch)..." << endl;
    ExternalPitchShiftStrategy externalStrategy(true, false);
    results.push_back(runBenchmark(&externalStrategy, input, semitones));

    cout << endl;
    cout << "===========================================================" << endl;
    cout << "                    Results Summary" << endl;
    cout << "===========================================================" << endl;
    cout << endl;

    // 결과 출력
    cout << left << setw(40) << "Algorithm"
         << right << setw(12) << "Time (ms)"
         << right << setw(15) << "Pitch Shift"
         << right << setw(15) << "Duration" << endl;
    cout << left << setw(40) << ""
         << right << setw(12) << ""
         << right << setw(15) << "(semitones)"
         << right << setw(15) << "Ratio" << endl;
    cout << string(82, '-') << endl;

    for (const auto& result : results) {
        cout << left << setw(40) << result.algorithmName
             << right << setw(12) << fixed << setprecision(3) << result.processingTimeMs
             << right << setw(15) << fixed << setprecision(2) << result.pitchShiftActual
             << right << setw(15) << fixed << setprecision(3) << result.durationRatio << endl;
    }

    cout << endl;

    // Pitch 정확도 분석
    cout << "Pitch Accuracy Analysis (Target: +" << semitones << " semitones):" << endl;
    for (const auto& result : results) {
        float error = result.pitchShiftActual - semitones;
        float errorPercent = (error / semitones) * 100.0f;
        cout << "  " << result.algorithmName << ":" << endl;
        cout << "    Original: " << fixed << setprecision(1) << result.originalPitch << " Hz" << endl;
        cout << "    Output:   " << fixed << setprecision(1) << result.outputPitch << " Hz" << endl;
        cout << "    Actual:   " << fixed << setprecision(2) << result.pitchShiftActual << " semitones";
        cout << " (Error: " << fixed << setprecision(2) << error << " semitones, "
             << fixed << setprecision(1) << errorPercent << "%)" << endl;
    }

    cout << endl;

    // 속도 비교 (Fast를 기준으로)
    double fastTime = results[0].processingTimeMs;
    cout << "Speed Comparison (relative to Fast):" << endl;
    for (const auto& result : results) {
        double ratio = result.processingTimeMs / fastTime;
        cout << "  " << result.algorithmName << ": "
             << fixed << setprecision(1) << ratio << "x" << endl;
    }

    cout << endl;

    // 품질 비교 (RMS 오차, 낮을수록 좋음)
    float fastRMS = results[0].rmsError;
    cout << "Quality Improvement (RMS error, lower is better):" << endl;
    for (const auto& result : results) {
        float improvement = ((fastRMS - result.rmsError) / fastRMS) * 100.0f;
        cout << "  " << result.algorithmName << ": ";
        if (improvement > 0) {
            cout << "+" << fixed << setprecision(1) << improvement << "% better";
        } else {
            cout << fixed << setprecision(1) << -improvement << "% worse";
        }
        cout << endl;
    }

    cout << endl;

    // WAV 파일 저장
    cout << "Saving output files..." << endl;
    wavFile.save("benchmark_fast.wav", results[0].outputBuffer);
    wavFile.save("benchmark_high.wav", results[1].outputBuffer);
    wavFile.save("benchmark_external.wav", results[2].outputBuffer);

    cout << "  - benchmark_fast.wav" << endl;
    cout << "  - benchmark_high.wav" << endl;
    cout << "  - benchmark_external.wav" << endl;

    cout << endl;
    cout << "Benchmark complete!" << endl;

    return 0;
}
