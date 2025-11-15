#include "../src/benchmark/PartialSegmentBenchmark.h"
#include "../src/utils/WaveFile.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.wav>" << endl;
        return 1;
    }

    string inputFile = argv[1];

    cout << "===========================================================" << endl;
    cout << "     Partial Segment Time Stretch Benchmark Report" << endl;
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
    cout << endl;

    // 테스트 구간 설정
    vector<float> segmentDurations = {0.5f, 1.0f, 2.0f};  // 500ms, 1s, 2s
    vector<float> ratios = {0.75f, 1.5f};  // 줄이기, 늘이기

    cout << "Test configurations:" << endl;
    cout << "  Segment durations: ";
    for (size_t i = 0; i < segmentDurations.size(); ++i) {
        cout << segmentDurations[i] << "s";
        if (i < segmentDurations.size() - 1) cout << ", ";
    }
    cout << endl;

    cout << "  Ratios: ";
    for (size_t i = 0; i < ratios.size(); ++i) {
        cout << ratios[i] << "x";
        if (i < ratios.size() - 1) cout << ", ";
    }
    cout << endl << endl;

    // 벤치마크 실행
    PartialSegmentBenchmark benchmark;
    auto results = benchmark.runAllBenchmarks(input, segmentDurations, ratios);

    // 결과 출력
    cout << endl;
    cout << "Results Summary:" << endl;
    cout << "----------------" << endl;

    // 구간 길이별로 그룹화
    for (float duration : segmentDurations) {
        cout << endl << "=== " << duration << "s Segment ===" << endl;

        for (float ratio : ratios) {
            cout << endl << ratio << "x (" << (ratio > 1.0f ? "stretch" : "compress") << "):" << endl;

            for (const auto& result : results) {
                if (result.segmentDuration == duration && result.targetRatio == ratio) {
                    cout << "  " << result.algorithmName << ":" << endl;
                    cout << "    Processing Time: " << result.processingTimeMs << " ms" << endl;
                    cout << "    Duration Error: " << result.durationError << "%" << endl;
                    cout << "    Boundary Discontinuity: " << result.boundaryDiscontinuity << endl;

                    // 경계 품질 평가
                    if (result.boundaryDiscontinuity < 0.01f) {
                        cout << "    Quality: ✅ Excellent boundary quality" << endl;
                    } else if (result.boundaryDiscontinuity < 0.05f) {
                        cout << "    Quality: ⚠️ Good boundary quality" << endl;
                    } else {
                        cout << "    Quality: ❌ Poor boundary quality (audible artifacts)" << endl;
                    }
                }
            }
        }
    }

    // 결과 디렉토리 생성
    system("mkdir -p ../benchmark_result");

    // HTML 리포트 저장
    string htmlReport = benchmark.resultsToHTML(results);
    ofstream htmlFile("../benchmark_result/benchmark_partialsegment_report.html");
    htmlFile << htmlReport;
    htmlFile.close();
    cout << endl << "HTML report saved to: ../benchmark_result/benchmark_partialsegment_report.html" << endl;

    // JSON 리포트 저장
    string jsonReport = benchmark.resultsToJSON(results);
    ofstream jsonFile("../benchmark_result/benchmark_partialsegment_report.json");
    jsonFile << jsonReport;
    jsonFile.close();
    cout << "JSON report saved to: ../benchmark_result/benchmark_partialsegment_report.json" << endl;

    // WAV 파일 저장 (각 알고리즘, 각 구간, 각 비율별)
    cout << endl << "Saving output files..." << endl;
    int idx = 0;
    for (const auto& result : results) {
        string outputFile = "../benchmark_result/output_partial_" + to_string(idx) + "_" +
                           to_string((int)(result.segmentDuration * 1000)) + "ms_" +
                           to_string((int)(result.targetRatio * 100)) + ".wav";
        wavFile.save(outputFile, result.outputAudio);
        cout << "  - " << outputFile << endl;
        cout << "    (" << result.algorithmName << ", " << result.segmentDuration << "s, "
             << result.targetRatio << "x)" << endl;
        idx++;
    }

    cout << endl << "Benchmark complete!" << endl;
    cout << "Total tests: " << results.size() << endl;
    cout << endl << "===========================================================" << endl;

    return 0;
}
