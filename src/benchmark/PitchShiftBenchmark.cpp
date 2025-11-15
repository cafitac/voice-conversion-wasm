#include "PitchShiftBenchmark.h"
#include "../effects/FastPitchShiftStrategy.h"
#include "../effects/HighQualityPitchShiftStrategy.h"
#include "../effects/ExternalPitchShiftStrategy.h"
#include "../effects/PSOLAPitchShiftStrategy.h"
#include "../effects/RubberBandPitchShiftStrategy.h"
#include "../analysis/PitchAnalyzer.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

PitchShiftBenchmark::PitchShiftBenchmark() {
}

PitchShiftBenchmark::~PitchShiftBenchmark() {
}

std::chrono::high_resolution_clock::time_point PitchShiftBenchmark::startTimer() {
    return std::chrono::high_resolution_clock::now();
}

double PitchShiftBenchmark::stopTimer(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0; // 밀리초로 변환
}

float PitchShiftBenchmark::measureAveragePitch(const AudioBuffer& buffer) {
    PitchAnalyzer analyzer;
    auto pitchPoints = analyzer.analyze(buffer, 0.02f);

    if (pitchPoints.empty()) {
        return 0.0f;
    }

    // Confidence 0.5 이상인 값만 사용
    std::vector<float> validPitches;
    for (const auto& point : pitchPoints) {
        if (point.confidence > 0.5f && point.frequency > 0.0f) {
            validPitches.push_back(point.frequency);
        }
    }

    if (validPitches.empty()) {
        return 0.0f;
    }

    // Median값 사용 (outlier 제거)
    std::sort(validPitches.begin(), validPitches.end());
    return validPitches[validPitches.size() / 2];
}

float PitchShiftBenchmark::hertzToSemitones(float originalHz, float targetHz) {
    if (originalHz <= 0.0f || targetHz <= 0.0f) {
        return 0.0f;
    }
    return 12.0f * std::log2(targetHz / originalHz);
}

double PitchShiftBenchmark::calculateRMSError(const AudioBuffer& original, const AudioBuffer& processed) {
    const auto& origData = original.getData();
    const auto& procData = processed.getData();

    // 길이가 다를 수 있으므로 최소 길이 사용
    size_t minSize = std::min(origData.size(), procData.size());

    double sumSquaredError = 0.0;
    for (size_t i = 0; i < minSize; ++i) {
        float error = procData[i] - origData[i];
        sumSquaredError += error * error;
    }

    double rmsError = std::sqrt(sumSquaredError / minSize);
    return rmsError;
}

PitchShiftMetrics PitchShiftBenchmark::runBenchmark(
    IPitchShiftStrategy* strategy,
    const AudioBuffer& input,
    float semitones
) {
    PitchShiftMetrics metrics;
    metrics.algorithmName = strategy->getName();
    metrics.semitones = semitones;

    // 입력 오디오 정보
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();
    int numSamples = input.getData().size() / channels;
    double audioDurationSec = static_cast<double>(numSamples) / sampleRate;

    // 원본 pitch 측정
    metrics.originalPitch = measureAveragePitch(input);

    // 처리 시간 측정
    auto startTime = startTimer();
    metrics.outputAudio = strategy->shiftPitch(input, semitones);
    metrics.processingTimeMs = stopTimer(startTime);

    // 성능 메트릭 계산
    metrics.throughputSamplesPerSec = (numSamples / metrics.processingTimeMs) * 1000.0;
    metrics.realtimeFactor = metrics.processingTimeMs / (audioDurationSec * 1000.0);

    // 출력 pitch 측정
    metrics.outputPitch = measureAveragePitch(metrics.outputAudio);

    // 실제 pitch shift 계산
    metrics.actualPitchSemitones = hertzToSemitones(metrics.originalPitch, metrics.outputPitch);
    metrics.pitchError = metrics.actualPitchSemitones - semitones;

    // Duration ratio 계산
    int outputNumSamples = metrics.outputAudio.getData().size() / channels;
    float outputDuration = static_cast<float>(outputNumSamples) / static_cast<float>(sampleRate);
    metrics.durationRatio = outputDuration / audioDurationSec;

    // RMS Error 계산
    metrics.rmsError = calculateRMSError(input, metrics.outputAudio);

    return metrics;
}

std::vector<PitchShiftMetrics> PitchShiftBenchmark::runAllBenchmarks(
    const AudioBuffer& input,
    const std::vector<float>& semitonesValues
) {
    std::vector<PitchShiftMetrics> results;

    // 5가지 알고리즘 생성
    std::vector<IPitchShiftStrategy*> strategies;
    strategies.push_back(new FastPitchShiftStrategy());
    strategies.push_back(new HighQualityPitchShiftStrategy(1024, 256));
    strategies.push_back(new ExternalPitchShiftStrategy(true, false));
    strategies.push_back(new PSOLAPitchShiftStrategy(2048, 512));
    strategies.push_back(new RubberBandPitchShiftStrategy(true, true));

    // 각 알고리즘과 semitones에 대해 벤치마크 실행
    for (auto* strategy : strategies) {
        for (float semitones : semitonesValues) {
            PitchShiftMetrics metrics = runBenchmark(strategy, input, semitones);
            results.push_back(metrics);
        }
    }

    // 메모리 정리
    for (auto* strategy : strategies) {
        delete strategy;
    }

    return results;
}

std::string PitchShiftBenchmark::escapeJSON(const std::string& input) {
    std::string output;
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    return output;
}

std::string PitchShiftBenchmark::resultsToJSON(const std::vector<PitchShiftMetrics>& results) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"benchmarkType\": \"PitchShift\",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    json << "  \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& m = results[i];
        json << "    {\n";
        json << "      \"algorithm\": \"" << escapeJSON(m.algorithmName) << "\",\n";
        json << "      \"semitones\": " << m.semitones << ",\n";
        json << "      \"processingTimeMs\": " << m.processingTimeMs << ",\n";
        json << "      \"throughputSamplesPerSec\": " << m.throughputSamplesPerSec << ",\n";
        json << "      \"realtimeFactor\": " << m.realtimeFactor << ",\n";
        json << "      \"originalPitch\": " << m.originalPitch << ",\n";
        json << "      \"outputPitch\": " << m.outputPitch << ",\n";
        json << "      \"actualPitchSemitones\": " << m.actualPitchSemitones << ",\n";
        json << "      \"pitchError\": " << m.pitchError << ",\n";
        json << "      \"durationRatio\": " << m.durationRatio << ",\n";
        json << "      \"rmsError\": " << m.rmsError << "\n";
        json << "    }";
        if (i < results.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";

    return json.str();
}

std::string PitchShiftBenchmark::resultsToHTML(const std::vector<PitchShiftMetrics>& results) {
    std::ostringstream html;
    html << std::fixed << std::setprecision(2);

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"ko\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>Pitch Shift Benchmark Report</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    html << "    .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "    h1 { color: #333; border-bottom: 3px solid #2196F3; padding-bottom: 10px; }\n";
    html << "    h2 { color: #555; margin-top: 30px; }\n";
    html << "    table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
    html << "    th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html << "    th { background-color: #2196F3; color: white; font-weight: bold; }\n";
    html << "    tr:hover { background-color: #f5f5f5; }\n";
    html << "    .summary { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 20px 0; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div class=\"container\">\n";
    html << "    <h1>Pitch Shift Benchmark Report</h1>\n";
    html << "    <p>Generated: " << std::time(nullptr) << "</p>\n";

    // Semitones별로 그룹화
    std::vector<float> semitonesValues;
    for (const auto& m : results) {
        if (std::find(semitonesValues.begin(), semitonesValues.end(), m.semitones) == semitonesValues.end()) {
            semitonesValues.push_back(m.semitones);
        }
    }

    for (float semitones : semitonesValues) {
        html << "    <h2>Pitch Shift: " << semitones << " semitones</h2>\n";
        html << "    <table>\n";
        html << "      <tr>\n";
        html << "        <th>Algorithm</th>\n";
        html << "        <th>Processing Time (ms)</th>\n";
        html << "        <th>Original Pitch (Hz)</th>\n";
        html << "        <th>Output Pitch (Hz)</th>\n";
        html << "        <th>Actual Shift (semitones)</th>\n";
        html << "        <th>Pitch Error (semitones)</th>\n";
        html << "        <th>Duration Ratio</th>\n";
        html << "      </tr>\n";

        // 해당 semitones의 결과만 필터링
        for (const auto& m : results) {
            if (std::abs(m.semitones - semitones) < 0.01f) {
                html << "      <tr>\n";
                html << "        <td>" << m.algorithmName << "</td>\n";
                html << "        <td>" << m.processingTimeMs << "</td>\n";
                html << "        <td>" << m.originalPitch << "</td>\n";
                html << "        <td>" << m.outputPitch << "</td>\n";
                html << "        <td>" << m.actualPitchSemitones << "</td>\n";
                html << "        <td>" << m.pitchError << "</td>\n";
                html << "        <td>" << m.durationRatio << "</td>\n";
                html << "      </tr>\n";
            }
        }

        html << "    </table>\n";
    }

    html << "    <div class=\"summary\">\n";
    html << "      <h2>Summary</h2>\n";
    html << "      <p><strong>Total Benchmarks:</strong> " << results.size() << "</p>\n";
    html << "    </div>\n";

    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}
