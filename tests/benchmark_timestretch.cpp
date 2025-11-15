#include "../src/benchmark/TimeStretchBenchmark.h"
#include "../src/utils/WaveFile.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.wav> [duration_ratio]" << endl;
        return 1;
    }

    string inputFile = argv[1];
    float durationRatio = (argc > 2) ? stof(argv[2]) : 1.5f;  // 기본값: 1.5x

    cout << "===========================================================" << endl;
    cout << "          Time Stretch Benchmark Report" << endl;
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
    cout << "Target Duration Ratio: " << durationRatio << "x" << endl;
    cout << endl;

    // 벤치마크 실행
    TimeStretchBenchmark benchmark;
    vector<float> ratioValues = {durationRatio};
    auto results = benchmark.runAllBenchmarks(input, ratioValues);

    // 결과 출력
    cout << "Results:" << endl;
    cout << "--------" << endl;
    for (const auto& result : results) {
        cout << result.algorithmName << ":" << endl;
        cout << "  Processing Time: " << result.processingTimeMs << " ms" << endl;
        cout << "  Original Duration: " << result.originalDuration << " seconds" << endl;
        cout << "  Output Duration: " << result.outputDuration << " seconds" << endl;
        cout << "  Actual Ratio: " << result.durationRatio << "x" << endl;
        cout << "  Duration Error: " << result.durationError << "%" << endl;
        cout << "  Pitch Change: " << result.pitchChangePercent << "%" << endl;
        cout << endl;
    }

    // 결과 디렉토리 생성
    system("mkdir -p ../benchmark_result");

    // HTML 리포트 저장
    string htmlReport = benchmark.resultsToHTML(results);
    ofstream htmlFile("../benchmark_result/benchmark_timestretch_report.html");
    htmlFile << htmlReport;
    htmlFile.close();
    cout << "HTML report saved to: ../benchmark_result/benchmark_timestretch_report.html" << endl;

    // JSON 리포트 저장
    string jsonReport = benchmark.resultsToJSON(results);
    ofstream jsonFile("../benchmark_result/benchmark_timestretch_report.json");
    jsonFile << jsonReport;
    jsonFile.close();
    cout << "JSON report saved to: ../benchmark_result/benchmark_timestretch_report.json" << endl;

    // WAV 파일 저장
    cout << "\nSaving output files..." << endl;
    int idx = 0;
    for (const auto& result : results) {
        string outputFile = "../benchmark_result/output_timestretch_" + to_string(idx) + ".wav";
        wavFile.save(outputFile, result.outputAudio);
        cout << "  - " << outputFile << " (" << result.algorithmName << ")" << endl;
        idx++;
    }

    cout << "\nBenchmark complete!" << endl;
    return 0;
}
