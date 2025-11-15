#include "../src/benchmark/CombinedBenchmark.h"
#include "../src/utils/WaveFile.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.wav> [semitones] [duration_ratio]" << endl;
        return 1;
    }

    string inputFile = argv[1];
    float semitones = (argc > 2) ? stof(argv[2]) : 3.0f;       // 기본값: +3 semitones
    float durationRatio = (argc > 3) ? stof(argv[3]) : 1.5f;  // 기본값: 1.5x

    cout << "===========================================================" << endl;
    cout << "     Combined (Pitch + Duration) Benchmark Report" << endl;
    cout << "===========================================================" << endl;
    cout << endl;

    // WAV 파일 로드
    WaveFile waveFile;
    AudioBuffer input = waveFile.load(inputFile);

    if (input.getData().empty()) {
        cerr << "Error: Failed to load " << inputFile << endl;
        return 1;
    }

    cout << "Input: " << inputFile << endl;
    cout << "Sample Rate: " << input.getSampleRate() << " Hz" << endl;
    cout << "Duration: " << input.getDuration() << " seconds" << endl;
    cout << "Target Pitch Shift: " << semitones << " semitones" << endl;
    cout << "Target Duration Ratio: " << durationRatio << "x" << endl;
    cout << endl;

    // 벤치마크 실행
    CombinedBenchmark benchmark;
    auto results = benchmark.runAllBenchmarks(input, semitones, durationRatio);

    // 결과 출력
    cout << "Results:" << endl;
    cout << "--------" << endl;
    for (const auto& result : results) {
        cout << result.methodName << ":" << endl;
        cout << "  Processing Time: " << result.processingTimeMs << " ms" << endl;
        cout << "  Pitch Error: " << result.pitchError << " semitones" << endl;
        cout << "  Duration Error: " << result.durationError << "%" << endl;
        cout << endl;
    }

    // 결과 디렉토리 생성
    system("mkdir -p ../benchmark_result");

    // HTML 리포트 저장
    string htmlReport = benchmark.resultsToHTML(results);
    ofstream htmlFile("../benchmark_result/benchmark_combined_report.html");
    htmlFile << htmlReport;
    htmlFile.close();
    cout << "HTML report saved to: ../benchmark_result/benchmark_combined_report.html" << endl;

    // JSON 리포트 저장
    string jsonReport = benchmark.resultsToJSON(results);
    ofstream jsonFile("../benchmark_result/benchmark_combined_report.json");
    jsonFile << jsonReport;
    jsonFile.close();
    cout << "JSON report saved to: ../benchmark_result/benchmark_combined_report.json" << endl;

    // WAV 파일 저장
    cout << "\nSaving output files..." << endl;
    int idx = 0;
    for (const auto& result : results) {
        string outputFile = "../benchmark_result/output_combined_" + to_string(idx) + ".wav";
        waveFile.save(outputFile, result.outputAudio);
        cout << "  - " << outputFile << " (" << result.methodName << ")" << endl;
        idx++;
    }

    cout << "\nBenchmark complete!" << endl;
    return 0;
}
