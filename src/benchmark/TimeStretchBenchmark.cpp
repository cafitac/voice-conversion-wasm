#include "TimeStretchBenchmark.h"
#include "../effects/FastTimeStretchStrategy.h"
#include "../effects/HighQualityTimeStretchStrategy.h"
#include "../effects/ExternalTimeStretchStrategy.h"
#include "../effects/PhaseVocoderTimeStretchStrategy.h"
#include "../effects/RubberBandTimeStretchStrategy.h"
#include "../analysis/PitchAnalyzer.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

TimeStretchBenchmark::TimeStretchBenchmark() {
}

TimeStretchBenchmark::~TimeStretchBenchmark() {
}

std::chrono::high_resolution_clock::time_point TimeStretchBenchmark::startTimer() {
    return std::chrono::high_resolution_clock::now();
}

double TimeStretchBenchmark::stopTimer(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0; // 밀리초로 변환
}

BenchmarkMetrics TimeStretchBenchmark::runBenchmark(
    ITimeStretchStrategy* strategy,
    const AudioBuffer& input,
    float ratio
) {
    BenchmarkMetrics metrics;
    metrics.algorithmName = strategy->getName();
    metrics.ratio = ratio;

    // 입력 오디오 정보
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();
    int numSamples = input.getData().size() / channels;
    double audioDurationSec = static_cast<double>(numSamples) / sampleRate;

    // 원본 pitch와 duration 측정
    PitchAnalyzer pitchAnalyzer;
    auto pitchPoints = pitchAnalyzer.analyze(input, 0.02f);

    // Median pitch 계산 (confidence > 0.5인 값만 사용)
    std::vector<float> validPitches;
    for (const auto& point : pitchPoints) {
        if (point.confidence > 0.5f && point.frequency > 0.0f) {
            validPitches.push_back(point.frequency);
        }
    }

    if (!validPitches.empty()) {
        std::sort(validPitches.begin(), validPitches.end());
        metrics.originalPitch = validPitches[validPitches.size() / 2];
    } else {
        metrics.originalPitch = 0.0f;
    }

    metrics.originalDuration = static_cast<float>(numSamples) / static_cast<float>(sampleRate);

    // 처리 시간 측정
    auto startTime = startTimer();
    metrics.outputAudio = strategy->stretch(input, ratio);
    metrics.processingTimeMs = stopTimer(startTime);

    // 성능 메트릭 계산
    metrics.throughputSamplesPerSec = (numSamples / metrics.processingTimeMs) * 1000.0;
    metrics.realtimeFactor = metrics.processingTimeMs / (audioDurationSec * 1000.0);

    // 출력 pitch와 duration 측정
    auto outputPitchPoints = pitchAnalyzer.analyze(metrics.outputAudio, 0.02f);
    std::vector<float> outputValidPitches;
    for (const auto& point : outputPitchPoints) {
        if (point.confidence > 0.5f && point.frequency > 0.0f) {
            outputValidPitches.push_back(point.frequency);
        }
    }

    if (!outputValidPitches.empty()) {
        std::sort(outputValidPitches.begin(), outputValidPitches.end());
        metrics.outputPitch = outputValidPitches[outputValidPitches.size() / 2];
    } else {
        metrics.outputPitch = 0.0f;
    }

    int outputNumSamples = metrics.outputAudio.getData().size() / channels;
    metrics.outputDuration = static_cast<float>(outputNumSamples) / static_cast<float>(sampleRate);

    // Duration ratio 계산
    metrics.durationRatio = metrics.outputDuration / metrics.originalDuration;
    metrics.durationError = ((metrics.durationRatio - ratio) / ratio) * 100.0f;

    // Pitch 변화 계산
    if (metrics.originalPitch > 0.0f) {
        metrics.pitchChangePercent = ((metrics.outputPitch - metrics.originalPitch) / metrics.originalPitch) * 100.0f;
    } else {
        metrics.pitchChangePercent = 0.0f;
    }

    // 품질 메트릭 계산 (ratio가 1.0에 가까울 때만 의미 있음)
    if (std::abs(ratio - 1.0f) < 0.01f) {
        metrics.snr = calculateSNR(input, metrics.outputAudio);
        metrics.rmsError = calculateRMSError(input, metrics.outputAudio);
    } else {
        metrics.snr = 0.0;
        metrics.rmsError = 0.0;
    }

    return metrics;
}

std::vector<BenchmarkMetrics> TimeStretchBenchmark::runAllBenchmarks(
    const AudioBuffer& input,
    const std::vector<float>& ratios
) {
    std::vector<BenchmarkMetrics> results;

    // 5가지 알고리즘 생성
    std::vector<ITimeStretchStrategy*> strategies;
    strategies.push_back(new FastTimeStretchStrategy());
    strategies.push_back(new HighQualityTimeStretchStrategy(1024, 256));
    strategies.push_back(new ExternalTimeStretchStrategy(true, false));
    strategies.push_back(new PhaseVocoderTimeStretchStrategy(2048, 512));
    strategies.push_back(new RubberBandTimeStretchStrategy());

    // 각 알고리즘과 ratio에 대해 벤치마크 실행
    for (auto* strategy : strategies) {
        for (float ratio : ratios) {
            BenchmarkMetrics metrics = runBenchmark(strategy, input, ratio);
            results.push_back(metrics);
        }
    }

    // 메모리 정리
    for (auto* strategy : strategies) {
        delete strategy;
    }

    return results;
}

double TimeStretchBenchmark::calculateSNR(const AudioBuffer& original, const AudioBuffer& processed) {
    const auto& origData = original.getData();
    const auto& procData = processed.getData();

    // 길이가 다르면 계산 불가
    if (origData.size() != procData.size()) {
        return 0.0;
    }

    // 신호 파워와 잡음 파워 계산
    double signalPower = 0.0;
    double noisePower = 0.0;

    for (size_t i = 0; i < origData.size(); ++i) {
        float original = origData[i];
        float processed = procData[i];
        float noise = processed - original;

        signalPower += original * original;
        noisePower += noise * noise;
    }

    // 0으로 나누기 방지
    if (noisePower < 1e-10) {
        return 100.0; // 매우 높은 SNR
    }

    // SNR (dB) = 10 * log10(signal_power / noise_power)
    double snr = 10.0 * std::log10(signalPower / noisePower);
    return snr;
}

double TimeStretchBenchmark::calculateRMSError(const AudioBuffer& original, const AudioBuffer& processed) {
    const auto& origData = original.getData();
    const auto& procData = processed.getData();

    // 길이가 다르면 계산 불가
    if (origData.size() != procData.size()) {
        return 0.0;
    }

    // RMS Error 계산
    double sumSquaredError = 0.0;
    for (size_t i = 0; i < origData.size(); ++i) {
        float error = procData[i] - origData[i];
        sumSquaredError += error * error;
    }

    double rmsError = std::sqrt(sumSquaredError / origData.size());
    return rmsError;
}

std::string TimeStretchBenchmark::escapeJSON(const std::string& input) {
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

std::string TimeStretchBenchmark::resultsToJSON(const std::vector<BenchmarkMetrics>& results) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"benchmarkType\": \"TimeStretch\",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    json << "  \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& m = results[i];
        json << "    {\n";
        json << "      \"algorithm\": \"" << escapeJSON(m.algorithmName) << "\",\n";
        json << "      \"ratio\": " << m.ratio << ",\n";
        json << "      \"processingTimeMs\": " << m.processingTimeMs << ",\n";
        json << "      \"throughputSamplesPerSec\": " << m.throughputSamplesPerSec << ",\n";
        json << "      \"realtimeFactor\": " << m.realtimeFactor << ",\n";
        json << "      \"snr\": " << m.snr << ",\n";
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

std::string TimeStretchBenchmark::resultsToHTML(const std::vector<BenchmarkMetrics>& results) {
    std::ostringstream html;
    html << std::fixed << std::setprecision(2);

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"ko\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>Time Stretch Benchmark Report</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    html << "    .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "    h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }\n";
    html << "    h2 { color: #555; margin-top: 30px; }\n";
    html << "    table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
    html << "    th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html << "    th { background-color: #4CAF50; color: white; font-weight: bold; }\n";
    html << "    tr:hover { background-color: #f5f5f5; }\n";
    html << "    .metric { font-weight: bold; color: #4CAF50; }\n";
    html << "    .summary { background: #e8f5e9; padding: 15px; border-radius: 5px; margin: 20px 0; }\n";
    html << "    .best { background-color: #c8e6c9; font-weight: bold; }\n";
    html << "    .worst { background-color: #ffcdd2; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div class=\"container\">\n";
    html << "    <h1>Time Stretch Benchmark Report</h1>\n";
    html << "    <p>Generated: " << std::time(nullptr) << "</p>\n";

    // 알고리즘별로 그룹화
    std::vector<std::string> algorithms;
    for (const auto& m : results) {
        if (std::find(algorithms.begin(), algorithms.end(), m.algorithmName) == algorithms.end()) {
            algorithms.push_back(m.algorithmName);
        }
    }

    // 각 ratio별 테이블 생성
    std::vector<float> ratios;
    for (const auto& m : results) {
        if (std::find(ratios.begin(), ratios.end(), m.ratio) == ratios.end()) {
            ratios.push_back(m.ratio);
        }
    }

    for (float ratio : ratios) {
        html << "    <h2>Ratio: " << ratio << "x</h2>\n";
        html << "    <table>\n";
        html << "      <tr>\n";
        html << "        <th>Algorithm</th>\n";
        html << "        <th>Processing Time (ms)</th>\n";
        html << "        <th>Throughput (samples/sec)</th>\n";
        html << "        <th>Real-time Factor</th>\n";
        if (std::abs(ratio - 1.0f) < 0.01f) {
            html << "        <th>SNR (dB)</th>\n";
            html << "        <th>RMS Error</th>\n";
        }
        html << "      </tr>\n";

        // 해당 ratio의 결과만 필터링
        for (const auto& m : results) {
            if (std::abs(m.ratio - ratio) < 0.01f) {
                html << "      <tr>\n";
                html << "        <td>" << m.algorithmName << "</td>\n";
                html << "        <td>" << m.processingTimeMs << "</td>\n";
                html << "        <td>" << m.throughputSamplesPerSec << "</td>\n";
                html << "        <td>" << m.realtimeFactor << "</td>\n";
                if (std::abs(ratio - 1.0f) < 0.01f) {
                    html << "        <td>" << m.snr << "</td>\n";
                    html << "        <td>" << m.rmsError << "</td>\n";
                }
                html << "      </tr>\n";
            }
        }

        html << "    </table>\n";
    }

    html << "    <div class=\"summary\">\n";
    html << "      <h2>Summary</h2>\n";
    html << "      <p><strong>Total Benchmarks:</strong> " << results.size() << "</p>\n";
    html << "      <p><strong>Algorithms Tested:</strong> " << algorithms.size() << "</p>\n";
    html << "      <p><strong>Ratios Tested:</strong> " << ratios.size() << "</p>\n";
    html << "    </div>\n";

    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}
