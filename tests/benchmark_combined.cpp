#include "../src/audio/AudioBuffer.h"
#include "../src/utils/WaveFile.h"
#include "../src/effects/ExternalPitchShiftStrategy.h"
#include "../src/effects/ExternalTimeStretchStrategy.h"
#include "../src/external/soundtouch/include/SoundTouch.h"
#include "../src/analysis/PitchAnalyzer.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <vector>

using namespace std;
using namespace std::chrono;

// Pitch 측정 (median값 사용)
float measureAveragePitch(const AudioBuffer& buffer) {
    PitchAnalyzer analyzer;
    auto pitchPoints = analyzer.analyze(buffer, 0.02f);

    if (pitchPoints.empty()) {
        return 0.0f;
    }

    vector<float> validPitches;
    for (const auto& point : pitchPoints) {
        if (point.confidence > 0.5f && point.frequency > 0.0f) {
            validPitches.push_back(point.frequency);
        }
    }

    if (validPitches.empty()) {
        return 0.0f;
    }

    sort(validPitches.begin(), validPitches.end());
    return validPitches[validPitches.size() / 2];
}

// Duration 측정
float measureDuration(const AudioBuffer& buffer) {
    return static_cast<float>(buffer.getData().size()) /
           static_cast<float>(buffer.getSampleRate() * buffer.getChannels());
}

// Semitones 계산
float hertzToSemitones(float originalHz, float targetHz) {
    if (originalHz <= 0.0f || targetHz <= 0.0f) return 0.0f;
    return 12.0f * log2(targetHz / originalHz);
}

// 벤치마크 결과 구조체
struct CombinedResult {
    string methodName;
    double processingTimeMs;
    AudioBuffer outputBuffer;

    // 목표값
    float targetPitchSemitones;
    float targetDurationRatio;

    // 측정값
    float originalPitch;
    float outputPitch;
    float actualPitchSemitones;
    float pitchError;  // semitones

    float originalDuration;
    float outputDuration;
    float actualDurationRatio;
    float durationError;  // %
};

// Method 1: Pitch shift 후 Time stretch (순차)
CombinedResult testSequentialPitchThenStretch(const AudioBuffer& input,
                                                float semitones,
                                                float durationRatio) {
    CombinedResult result;
    result.methodName = "Sequential: Pitch then TimeStretch";
    result.targetPitchSemitones = semitones;
    result.targetDurationRatio = durationRatio;

    result.originalPitch = measureAveragePitch(input);
    result.originalDuration = measureDuration(input);

    auto startTime = high_resolution_clock::now();

    // 1. Pitch shift
    ExternalPitchShiftStrategy pitchShifter;
    AudioBuffer afterPitch = pitchShifter.shiftPitch(input, semitones);

    // 2. Time stretch
    ExternalTimeStretchStrategy timeStretcher;
    result.outputBuffer = timeStretcher.stretch(afterPitch, durationRatio);

    auto endTime = high_resolution_clock::now();
    result.processingTimeMs = duration_cast<microseconds>(endTime - startTime).count() / 1000.0;

    result.outputPitch = measureAveragePitch(result.outputBuffer);
    result.outputDuration = measureDuration(result.outputBuffer);

    result.actualPitchSemitones = hertzToSemitones(result.originalPitch, result.outputPitch);
    result.pitchError = result.actualPitchSemitones - semitones;

    result.actualDurationRatio = result.outputDuration / result.originalDuration;
    result.durationError = ((result.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return result;
}

// Method 2: Time stretch 후 Pitch shift (순차)
CombinedResult testSequentialStretchThenPitch(const AudioBuffer& input,
                                                float semitones,
                                                float durationRatio) {
    CombinedResult result;
    result.methodName = "Sequential: TimeStretch then Pitch";
    result.targetPitchSemitones = semitones;
    result.targetDurationRatio = durationRatio;

    result.originalPitch = measureAveragePitch(input);
    result.originalDuration = measureDuration(input);

    auto startTime = high_resolution_clock::now();

    // 1. Time stretch
    ExternalTimeStretchStrategy timeStretcher;
    AudioBuffer afterStretch = timeStretcher.stretch(input, durationRatio);

    // 2. Pitch shift
    ExternalPitchShiftStrategy pitchShifter;
    result.outputBuffer = pitchShifter.shiftPitch(afterStretch, semitones);

    auto endTime = high_resolution_clock::now();
    result.processingTimeMs = duration_cast<microseconds>(endTime - startTime).count() / 1000.0;

    result.outputPitch = measureAveragePitch(result.outputBuffer);
    result.outputDuration = measureDuration(result.outputBuffer);

    result.actualPitchSemitones = hertzToSemitones(result.originalPitch, result.outputPitch);
    result.pitchError = result.actualPitchSemitones - semitones;

    result.actualDurationRatio = result.outputDuration / result.originalDuration;
    result.durationError = ((result.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return result;
}

// Method 3: SoundTouch 직접 사용 (한 번에)
CombinedResult testSoundTouchDirect(const AudioBuffer& input,
                                    float semitones,
                                    float durationRatio) {
    CombinedResult result;
    result.methodName = "Direct: SoundTouch Combined";
    result.targetPitchSemitones = semitones;
    result.targetDurationRatio = durationRatio;

    result.originalPitch = measureAveragePitch(input);
    result.originalDuration = measureDuration(input);

    auto startTime = high_resolution_clock::now();

    // SoundTouch에 pitch와 tempo를 동시에 설정
    soundtouch::SoundTouch st;
    st.setSampleRate(input.getSampleRate());
    st.setChannels(input.getChannels());
    st.setPitchSemiTones(semitones);
    st.setTempo(1.0 / durationRatio);  // tempo = 1 / ratio

    const auto& inputData = input.getData();
    st.putSamples(inputData.data(), inputData.size() / input.getChannels());
    st.flush();

    vector<float> outputData;
    const int BUFFER_SIZE = 4096;
    vector<float> tempBuffer(BUFFER_SIZE * input.getChannels());

    int numSamples;
    while ((numSamples = st.receiveSamples(tempBuffer.data(), BUFFER_SIZE)) > 0) {
        outputData.insert(outputData.end(),
                         tempBuffer.begin(),
                         tempBuffer.begin() + numSamples * input.getChannels());
    }

    result.outputBuffer = AudioBuffer(input.getSampleRate(), input.getChannels());
    result.outputBuffer.setData(outputData);

    auto endTime = high_resolution_clock::now();
    result.processingTimeMs = duration_cast<microseconds>(endTime - startTime).count() / 1000.0;

    result.outputPitch = measureAveragePitch(result.outputBuffer);
    result.outputDuration = measureDuration(result.outputBuffer);

    result.actualPitchSemitones = hertzToSemitones(result.originalPitch, result.outputPitch);
    result.pitchError = result.actualPitchSemitones - semitones;

    result.actualDurationRatio = result.outputDuration / result.originalDuration;
    result.durationError = ((result.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return result;
}

// 결과 출력
void printResults(const vector<CombinedResult>& results) {
    cout << "\n========================================" << endl;
    cout << "Pitch + Duration 결합 처리 비교 결과" << endl;
    cout << "========================================\n" << endl;

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];

        cout << (i + 1) << ". " << r.methodName << endl;
        cout << string(60, '-') << endl;

        // 처리 시간
        cout << "처리 시간: " << fixed << setprecision(3)
             << r.processingTimeMs << " ms" << endl;

        // Pitch 정확도
        cout << "\nPitch 정확도:" << endl;
        cout << "  - 원본 Pitch: " << fixed << setprecision(1)
             << r.originalPitch << " Hz" << endl;
        cout << "  - 출력 Pitch: " << fixed << setprecision(1)
             << r.outputPitch << " Hz" << endl;
        cout << "  - 목표 Shift: " << fixed << setprecision(2)
             << r.targetPitchSemitones << " semitones" << endl;
        cout << "  - 실제 Shift: " << fixed << setprecision(2)
             << r.actualPitchSemitones << " semitones" << endl;
        cout << "  - Pitch 오차: " << fixed << setprecision(2)
             << r.pitchError << " semitones" << endl;

        if (abs(r.pitchError) < 0.1f) {
            cout << "  ✅ 평가: Pitch 매우 정확" << endl;
        } else if (abs(r.pitchError) < 0.3f) {
            cout << "  ✅ 평가: Pitch 정확" << endl;
        } else if (abs(r.pitchError) < 0.5f) {
            cout << "  ⚠️  평가: Pitch 약간 부정확" << endl;
        } else {
            cout << "  ❌ 평가: Pitch 부정확" << endl;
        }

        // Duration 정확도
        cout << "\nDuration 정확도:" << endl;
        cout << "  - 원본 Duration: " << fixed << setprecision(3)
             << r.originalDuration << " seconds" << endl;
        cout << "  - 출력 Duration: " << fixed << setprecision(3)
             << r.outputDuration << " seconds" << endl;
        cout << "  - 목표 Ratio: " << fixed << setprecision(2)
             << r.targetDurationRatio << "x" << endl;
        cout << "  - 실제 Ratio: " << fixed << setprecision(3)
             << r.actualDurationRatio << "x" << endl;
        cout << "  - Duration 오차: " << fixed << setprecision(2)
             << r.durationError << "%" << endl;

        if (abs(r.durationError) < 1.0f) {
            cout << "  ✅ 평가: Duration 매우 정확" << endl;
        } else if (abs(r.durationError) < 3.0f) {
            cout << "  ✅ 평가: Duration 정확" << endl;
        } else if (abs(r.durationError) < 5.0f) {
            cout << "  ⚠️  평가: Duration 약간 부정확" << endl;
        } else {
            cout << "  ❌ 평가: Duration 부정확" << endl;
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
         [](const CombinedResult& a, const CombinedResult& b) {
             return a.processingTimeMs < b.processingTimeMs;
         });

    for (size_t i = 0; i < speedResults.size(); ++i) {
        cout << "  " << (i + 1) << ". " << speedResults[i].methodName
             << " - " << fixed << setprecision(3)
             << speedResults[i].processingTimeMs << " ms" << endl;
    }

    // Pitch 정확도 순위
    cout << "\nPitch 정확도 순위:" << endl;
    auto pitchResults = results;
    sort(pitchResults.begin(), pitchResults.end(),
         [](const CombinedResult& a, const CombinedResult& b) {
             return abs(a.pitchError) < abs(b.pitchError);
         });

    for (size_t i = 0; i < pitchResults.size(); ++i) {
        cout << "  " << (i + 1) << ". " << pitchResults[i].methodName
             << " - 오차 " << fixed << setprecision(3)
             << abs(pitchResults[i].pitchError) << " semitones" << endl;
    }

    // Duration 정확도 순위
    cout << "\nDuration 정확도 순위:" << endl;
    auto durationResults = results;
    sort(durationResults.begin(), durationResults.end(),
         [](const CombinedResult& a, const CombinedResult& b) {
             return abs(a.durationError) < abs(b.durationError);
         });

    for (size_t i = 0; i < durationResults.size(); ++i) {
        cout << "  " << (i + 1) << ". " << durationResults[i].methodName
             << " - 오차 " << fixed << setprecision(2)
             << abs(durationResults[i].durationError) << "%" << endl;
    }

    // 추천 방식
    cout << "\n추천 방식:" << endl;
    cout << "  🥇 속도 최우선: " << speedResults[0].methodName << endl;
    cout << "  🥇 정확도 최우선: " << pitchResults[0].methodName << endl;
}

int main(int argc, char* argv[]) {
    cout << "Pitch + Duration 결합 처리 벤치마크\n" << endl;

    // 입력 파일 확인
    string inputFile = (argc > 1) ? argv[1] : "benchmark_fast.wav";
    float semitones = (argc > 2) ? stof(argv[2]) : 3.0f;  // 기본값: +3 semitones
    float durationRatio = (argc > 3) ? stof(argv[3]) : 1.5f;  // 기본값: 1.5x

    cout << "입력 파일: " << inputFile << endl;
    cout << "목표 Pitch Shift: " << fixed << setprecision(1) << semitones << " semitones" << endl;
    cout << "목표 Duration Ratio: " << fixed << setprecision(2) << durationRatio << "x" << endl;
    cout << endl;

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
    vector<CombinedResult> results;

    cout << "Testing Method 1: Pitch then TimeStretch..." << endl;
    results.push_back(testSequentialPitchThenStretch(input, semitones, durationRatio));

    cout << "Testing Method 2: TimeStretch then Pitch..." << endl;
    results.push_back(testSequentialStretchThenPitch(input, semitones, durationRatio));

    cout << "Testing Method 3: SoundTouch Direct..." << endl;
    results.push_back(testSoundTouchDirect(input, semitones, durationRatio));

    // 결과 출력
    printResults(results);

    // 결과 파일 저장
    cout << "\n결과 파일 저장 중..." << endl;
    WaveFile outputWaveFile;
    outputWaveFile.save("output_combined_pitch_then_stretch.wav", results[0].outputBuffer);
    outputWaveFile.save("output_combined_stretch_then_pitch.wav", results[1].outputBuffer);
    outputWaveFile.save("output_combined_direct.wav", results[2].outputBuffer);
    cout << "  - output_combined_pitch_then_stretch.wav" << endl;
    cout << "  - output_combined_stretch_then_pitch.wav" << endl;
    cout << "  - output_combined_direct.wav" << endl;

    cout << "\n✓ 벤치마크 완료!" << endl;
    return 0;
}
