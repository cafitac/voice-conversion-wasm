#include "PartialSegmentBenchmark.h"
#include "../effects/FastTimeStretchStrategy.h"
#include "../effects/HighQualityTimeStretchStrategy.h"
#include "../effects/ExternalTimeStretchStrategy.h"
#include "../effects/PhaseVocoderTimeStretchStrategy.h"
#include "../effects/RubberBandTimeStretchStrategy.h"
#include "../analysis/PitchAnalyzer.h"
#include "../analysis/DurationAnalyzer.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

PartialSegmentBenchmark::PartialSegmentBenchmark() {
}

PartialSegmentBenchmark::~PartialSegmentBenchmark() {
}

std::chrono::high_resolution_clock::time_point PartialSegmentBenchmark::startTimer() {
    return std::chrono::high_resolution_clock::now();
}

double PartialSegmentBenchmark::stopTimer(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0;  // Convert to milliseconds
}

AudioBuffer PartialSegmentBenchmark::processSegment(
    const AudioBuffer& input,
    ITimeStretchStrategy* strategy,
    int startSample,
    int endSample,
    float ratio
) {
    const auto& inputData = input.getData();
    int channels = input.getChannels();
    int sampleRate = input.getSampleRate();

    // 1. 구간 추출
    std::vector<float> segmentData;
    for (int i = startSample * channels; i < endSample * channels && i < (int)inputData.size(); ++i) {
        segmentData.push_back(inputData[i]);
    }

    AudioBuffer segment(sampleRate, channels);
    segment.setData(segmentData);

    // 2. 구간 처리
    AudioBuffer processedSegment = strategy->stretch(segment, ratio);

    // 3. 전체 오디오에 합성
    std::vector<float> output;

    // 앞부분 (원본)
    for (int i = 0; i < startSample * channels; ++i) {
        output.push_back(inputData[i]);
    }

    // 처리된 구간 (Crossfade 적용)
    const auto& processedData = processedSegment.getData();
    int fadeLength = std::min(1440, (int)processedData.size() / 10);  // 30ms or 10% of segment

    // Crossfade in at start
    for (int i = 0; i < fadeLength && i < (int)processedData.size(); ++i) {
        float alpha = (float)i / fadeLength;
        int originalIdx = (startSample * channels) + i;
        float originalSample = (originalIdx < (int)inputData.size()) ? inputData[originalIdx] : 0.0f;
        output.push_back(originalSample * (1.0f - alpha) + processedData[i] * alpha);
    }

    // Middle part (full processed)
    for (int i = fadeLength; i < (int)processedData.size() - fadeLength; ++i) {
        output.push_back(processedData[i]);
    }

    // Crossfade out at end
    for (int i = std::max(0, (int)processedData.size() - fadeLength); i < (int)processedData.size(); ++i) {
        float alpha = (float)(processedData.size() - i) / fadeLength;
        int originalIdx = (endSample * channels) + (i - ((int)processedData.size() - fadeLength));
        float originalSample = (originalIdx < (int)inputData.size()) ? inputData[originalIdx] : 0.0f;
        output.push_back(processedData[i] * alpha + originalSample * (1.0f - alpha));
    }

    // 뒷부분 (원본)
    int endIdx = endSample * channels + (int)processedData.size() - fadeLength;
    for (int i = endIdx; i < (int)inputData.size(); ++i) {
        output.push_back(inputData[i]);
    }

    AudioBuffer result(sampleRate, channels);
    result.setData(output);
    return result;
}

float PartialSegmentBenchmark::measureBoundaryDiscontinuity(
    const AudioBuffer& audio,
    int boundarySample,
    int windowSize
) {
    const auto& data = audio.getData();
    int channels = audio.getChannels();

    if (boundarySample <= 0 || boundarySample >= (int)(data.size() / channels)) {
        return 0.0f;
    }

    // 경계 전후의 차이 측정
    float sumSquaredDiff = 0.0f;
    int count = 0;

    for (int i = 0; i < windowSize; ++i) {
        int idx1 = (boundarySample - i - 1) * channels;
        int idx2 = (boundarySample + i) * channels;

        if (idx1 >= 0 && idx2 < (int)data.size()) {
            float diff = data[idx2] - data[idx1];
            sumSquaredDiff += diff * diff;
            count++;
        }
    }

    return (count > 0) ? std::sqrt(sumSquaredDiff / count) : 0.0f;
}

float PartialSegmentBenchmark::measureBoundarySmoothness(
    const AudioBuffer& audio,
    int boundarySample,
    int windowSize
) {
    const auto& data = audio.getData();
    int channels = audio.getChannels();

    if (boundarySample <= 0 || boundarySample >= (int)(data.size() / channels)) {
        return 0.0f;
    }

    // 경계 주변의 변화율 측정 (1차 미분)
    float sumAbsDiff = 0.0f;
    int count = 0;

    int startIdx = std::max(0, boundarySample - windowSize);
    int endIdx = std::min((int)(data.size() / channels), boundarySample + windowSize);

    for (int i = startIdx; i < endIdx - 1; ++i) {
        int idx1 = i * channels;
        int idx2 = (i + 1) * channels;

        if (idx2 < (int)data.size()) {
            sumAbsDiff += std::abs(data[idx2] - data[idx1]);
            count++;
        }
    }

    return (count > 0) ? (sumAbsDiff / count) : 0.0f;
}

SegmentMetrics PartialSegmentBenchmark::runBenchmark(
    ITimeStretchStrategy* strategy,
    const AudioBuffer& input,
    float segmentStart,
    float segmentDuration,
    float ratio
) {
    SegmentMetrics metrics;
    metrics.algorithmName = strategy->getName();
    metrics.segmentStart = segmentStart;
    metrics.segmentDuration = segmentDuration;
    metrics.segmentEnd = segmentStart + segmentDuration;
    metrics.targetRatio = ratio;

    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();

    // 샘플 인덱스 계산
    int startSample = (int)(segmentStart * sampleRate);
    int endSample = (int)((segmentStart + segmentDuration) * sampleRate);

    // 원본 구간 길이
    metrics.originalSegmentDuration = segmentDuration;

    // 처리 시간 측정
    auto start = startTimer();
    metrics.outputAudio = processSegment(input, strategy, startSample, endSample, ratio);
    metrics.processingTimeMs = stopTimer(start);

    // Real-time factor 계산
    float audioDuration = segmentDuration;
    metrics.realtimeFactor = metrics.processingTimeMs / (audioDuration * 1000.0);

    // 출력 구간 길이 추정
    // 처리된 부분만 ratio가 적용되므로 전체 길이에서 계산
    float expectedOutputDuration = segmentDuration * ratio;
    metrics.outputSegmentDuration = expectedOutputDuration;
    metrics.actualRatio = expectedOutputDuration / segmentDuration;
    metrics.durationError = ((metrics.actualRatio - ratio) / ratio) * 100.0f;

    // 경계 품질 측정
    int processedStart = startSample;
    int processedEnd = processedStart + (int)(expectedOutputDuration * sampleRate);

    metrics.leftBoundarySmoothl = measureBoundarySmoothness(metrics.outputAudio, processedStart, 100);
    metrics.rightBoundarySmoothness = measureBoundarySmoothness(metrics.outputAudio, processedEnd, 100);
    metrics.boundaryDiscontinuity = (
        measureBoundaryDiscontinuity(metrics.outputAudio, processedStart, 50) +
        measureBoundaryDiscontinuity(metrics.outputAudio, processedEnd, 50)
    ) / 2.0f;

    // 설명 생성
    std::ostringstream desc;
    desc << std::fixed << std::setprecision(2);
    desc << segmentDuration << "s segment, ";
    desc << ratio << "x " << (ratio > 1.0f ? "stretch" : "compress");
    metrics.description = desc.str();

    return metrics;
}

std::vector<SegmentMetrics> PartialSegmentBenchmark::runAllBenchmarks(
    const AudioBuffer& input,
    const std::vector<float>& segmentDurations,
    const std::vector<float>& ratios
) {
    std::vector<SegmentMetrics> results;

    // 알고리즘 목록
    std::vector<ITimeStretchStrategy*> strategies;
    strategies.push_back(new FastTimeStretchStrategy());
    strategies.push_back(new HighQualityTimeStretchStrategy(1024, 256));
    strategies.push_back(new ExternalTimeStretchStrategy(true, false));
    strategies.push_back(new PhaseVocoderTimeStretchStrategy(2048, 512));
    strategies.push_back(new RubberBandTimeStretchStrategy());

    // 오디오 중간 지점에서 시작
    float totalDuration = input.getDuration();
    float maxSegmentDuration = *std::max_element(segmentDurations.begin(), segmentDurations.end());
    float startTime = (totalDuration - maxSegmentDuration) / 2.0f;

    // 모든 조합 테스트
    for (auto strategy : strategies) {
        for (float segmentDuration : segmentDurations) {
            for (float ratio : ratios) {
                printf("Testing %s: %.1fs segment, %.2fx ratio\n",
                       strategy->getName(), segmentDuration, ratio);

                SegmentMetrics metrics = runBenchmark(
                    strategy,
                    input,
                    startTime,
                    segmentDuration,
                    ratio
                );

                results.push_back(metrics);
            }
        }
    }

    // Clean up
    for (auto strategy : strategies) {
        delete strategy;
    }

    return results;
}

std::string PartialSegmentBenchmark::escapeJSON(const std::string& input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::string PartialSegmentBenchmark::resultsToJSON(const std::vector<SegmentMetrics>& results) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"benchmarkType\": \"PartialSegment\",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    json << "  \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];

        json << "    {\n";
        json << "      \"algorithm\": \"" << escapeJSON(r.algorithmName) << "\",\n";
        json << "      \"description\": \"" << escapeJSON(r.description) << "\",\n";
        json << "      \"segmentDuration\": " << r.segmentDuration << ",\n";
        json << "      \"segmentStart\": " << r.segmentStart << ",\n";
        json << "      \"segmentEnd\": " << r.segmentEnd << ",\n";
        json << "      \"targetRatio\": " << r.targetRatio << ",\n";
        json << "      \"processingTimeMs\": " << r.processingTimeMs << ",\n";
        json << "      \"realtimeFactor\": " << r.realtimeFactor << ",\n";
        json << "      \"originalSegmentDuration\": " << r.originalSegmentDuration << ",\n";
        json << "      \"outputSegmentDuration\": " << r.outputSegmentDuration << ",\n";
        json << "      \"actualRatio\": " << r.actualRatio << ",\n";
        json << "      \"durationError\": " << r.durationError << ",\n";
        json << "      \"boundaryDiscontinuity\": " << r.boundaryDiscontinuity << ",\n";
        json << "      \"leftBoundarySmoothness\": " << r.leftBoundarySmoothl << ",\n";
        json << "      \"rightBoundarySmoothness\": " << r.rightBoundarySmoothness << "\n";
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

std::string PartialSegmentBenchmark::resultsToHTML(const std::vector<SegmentMetrics>& results) {
    std::ostringstream html;
    html << std::fixed << std::setprecision(2);

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"ko\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>Partial Segment Time Stretch Benchmark Report</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    html << "    .container { max-width: 1400px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "    h1 { color: #333; border-bottom: 3px solid #2196F3; padding-bottom: 10px; }\n";
    html << "    h2 { color: #555; margin-top: 30px; }\n";
    html << "    table { width: 100%; border-collapse: collapse; margin-top: 20px; font-size: 0.9em; }\n";
    html << "    th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html << "    th { background-color: #2196F3; color: white; font-weight: bold; position: sticky; top: 0; }\n";
    html << "    tr:hover { background-color: #f5f5f5; }\n";
    html << "    .good { color: #4CAF50; font-weight: bold; }\n";
    html << "    .warning { color: #FF9800; font-weight: bold; }\n";
    html << "    .bad { color: #f44336; font-weight: bold; }\n";
    html << "    .summary { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 20px 0; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div class=\"container\">\n";
    html << "    <h1>부분 구간 시간 늘이기 벤치마크 보고서</h1>\n";
    html << "    <p>Generated: " << std::time(nullptr) << "</p>\n";
    html << "    <div class=\"summary\">\n";
    html << "      <h3>테스트 개요</h3>\n";
    html << "      <p><strong>목적:</strong> 전체 오디오가 아닌 특정 구간만 시간을 늘이거나 줄일 때의 성능과 품질 검증</p>\n";
    html << "      <p><strong>실제 사용 사례:</strong> 음성 편집, 특정 단어/구간 속도 조절, 리듬 조정</p>\n";
    html << "      <p><strong>총 테스트 수:</strong> " << results.size() << "개</p>\n";
    html << "    </div>\n";

    html << "    <h2>전체 결과</h2>\n";
    html << "    <table>\n";
    html << "      <tr>\n";
    html << "        <th>알고리즘</th>\n";
    html << "        <th>구간 길이</th>\n";
    html << "        <th>비율</th>\n";
    html << "        <th>처리 시간 (ms)</th>\n";
    html << "        <th>실시간 처리</th>\n";
    html << "        <th>길이 오차</th>\n";
    html << "        <th>경계 불연속성</th>\n";
    html << "        <th>경계 부드러움</th>\n";
    html << "      </tr>\n";

    for (const auto& r : results) {
        html << "      <tr>\n";
        html << "        <td>" << r.algorithmName << "</td>\n";
        html << "        <td>" << r.segmentDuration << "s</td>\n";
        html << "        <td>" << r.targetRatio << "x</td>\n";
        html << "        <td>" << r.processingTimeMs << "</td>\n";

        // 실시간 처리 가능 여부
        if (r.realtimeFactor < 0.1) {
            html << "        <td class=\"good\">✅ " << r.realtimeFactor << "x</td>\n";
        } else if (r.realtimeFactor < 0.5) {
            html << "        <td class=\"warning\">⚠️ " << r.realtimeFactor << "x</td>\n";
        } else {
            html << "        <td class=\"bad\">❌ " << r.realtimeFactor << "x</td>\n";
        }

        // 길이 오차
        if (std::abs(r.durationError) < 1.0f) {
            html << "        <td class=\"good\">" << r.durationError << "%</td>\n";
        } else if (std::abs(r.durationError) < 5.0f) {
            html << "        <td class=\"warning\">" << r.durationError << "%</td>\n";
        } else {
            html << "        <td class=\"bad\">" << r.durationError << "%</td>\n";
        }

        // 경계 불연속성 (낮을수록 좋음)
        if (r.boundaryDiscontinuity < 0.01f) {
            html << "        <td class=\"good\">" << r.boundaryDiscontinuity << "</td>\n";
        } else if (r.boundaryDiscontinuity < 0.05f) {
            html << "        <td class=\"warning\">" << r.boundaryDiscontinuity << "</td>\n";
        } else {
            html << "        <td class=\"bad\">" << r.boundaryDiscontinuity << "</td>\n";
        }

        // 평균 경계 부드러움
        float avgSmoothness = (r.leftBoundarySmoothl + r.rightBoundarySmoothness) / 2.0f;
        html << "        <td>" << avgSmoothness << "</td>\n";

        html << "      </tr>\n";
    }

    html << "    </table>\n";
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}
