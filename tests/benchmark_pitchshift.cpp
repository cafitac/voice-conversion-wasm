#include "../src/benchmark/PitchShiftBenchmark.h"
#include "../src/utils/WaveFile.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.wav> [semitones]" << endl;
        return 1;
    }

    string inputFile = argv[1];
    float semitones = (argc > 2) ? stof(argv[2]) : 3.0f;  // 기본값: +3 semitones

    cout << "===========================================================" << endl;
    cout << "          Pitch Shift Benchmark Report" << endl;
    cout << "===========================================================" << endl;
    cout << endl;

    // WAV 파일 로드
    WaveFile wavFile;
    AudioBuffer input = wavFile.load(inputFile);

    if (input.getData().empty()) {
        cerr << "Error: Failed to load " << inputFile << endl;
        return 1;
    }

    cout << "Input: " << inputFile << endl;
    cout << "Sample Rate: " << input.getSampleRate() << " Hz" << endl;
    cout << "Duration: " << input.getDuration() << " seconds" << endl;
    cout << "Pitch Shift: +" << semitones << " semitones" << endl;
    cout << endl;

    // 벤치마크 실행
    PitchShiftBenchmark benchmark;
    vector<float> semitonesValues = {semitones};
    auto results = benchmark.runAllBenchmarks(input, semitonesValues);

    // 결과 출력
    cout << "Results:" << endl;
    cout << "--------" << endl;
    for (const auto& result : results) {
        cout << result.algorithmName << ":" << endl;
        cout << "  Processing Time: " << result.processingTimeMs << " ms" << endl;
        cout << "  Original Pitch: " << result.originalPitch << " Hz" << endl;
        cout << "  Output Pitch: " << result.outputPitch << " Hz" << endl;
        cout << "  Actual Shift: " << result.actualPitchSemitones << " semitones" << endl;
        cout << "  Pitch Error: " << result.pitchError << " semitones" << endl;
        cout << "  Duration Ratio: " << result.durationRatio << endl;
        cout << endl;
    }

    // 결과 디렉토리 생성
    system("mkdir -p ../benchmark_result");

    // HTML 리포트 저장
    string htmlReport = benchmark.resultsToHTML(results);
    ofstream htmlFile("../benchmark_result/benchmark_pitchshift_report.html");
    htmlFile << htmlReport;
    htmlFile.close();
    cout << "HTML report saved to: ../benchmark_result/benchmark_pitchshift_report.html" << endl;

    // JSON 리포트 저장
    string jsonReport = benchmark.resultsToJSON(results);
    ofstream jsonFile("../benchmark_result/benchmark_pitchshift_report.json");
    jsonFile << jsonReport;
    jsonFile.close();
    cout << "JSON report saved to: ../benchmark_result/benchmark_pitchshift_report.json" << endl;

    // WAV 파일 저장
    cout << "\nSaving output files..." << endl;
    int idx = 0;
    for (const auto& result : results) {
        string outputFile = "../benchmark_result/output_pitch_" + to_string(idx) + ".wav";
        wavFile.save(outputFile, result.outputAudio);
        cout << "  - " << outputFile << " (" << result.algorithmName << ")" << endl;
        idx++;
    }

    cout << "\nBenchmark complete!" << endl;
    return 0;
}
