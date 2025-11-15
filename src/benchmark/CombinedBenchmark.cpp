#include "CombinedBenchmark.h"
#include "../effects/ExternalPitchShiftStrategy.h"
#include "../effects/ExternalTimeStretchStrategy.h"
#include "../effects/HighQualityPitchShiftStrategy.h"
#include "../effects/PhaseVocoderTimeStretchStrategy.h"
#include "../external/soundtouch/include/SoundTouch.h"
#include "../external/rubberband/rubberband/rubberband-c.h"
#include "../analysis/PitchAnalyzer.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

CombinedBenchmark::CombinedBenchmark() {
}

CombinedBenchmark::~CombinedBenchmark() {
}

std::chrono::high_resolution_clock::time_point CombinedBenchmark::startTimer() {
    return std::chrono::high_resolution_clock::now();
}

double CombinedBenchmark::stopTimer(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0; // 밀리초로 변환
}

float CombinedBenchmark::measureAveragePitch(const AudioBuffer& buffer) {
    PitchAnalyzer analyzer;
    auto pitchPoints = analyzer.analyze(buffer, 0.02f);

    if (pitchPoints.empty()) {
        return 0.0f;
    }

    std::vector<float> validPitches;
    for (const auto& point : pitchPoints) {
        if (point.confidence > 0.5f && point.frequency > 0.0f) {
            validPitches.push_back(point.frequency);
        }
    }

    if (validPitches.empty()) {
        return 0.0f;
    }

    std::sort(validPitches.begin(), validPitches.end());
    return validPitches[validPitches.size() / 2];
}

float CombinedBenchmark::measureDuration(const AudioBuffer& buffer) {
    return static_cast<float>(buffer.getData().size()) /
           static_cast<float>(buffer.getSampleRate() * buffer.getChannels());
}

float CombinedBenchmark::hertzToSemitones(float originalHz, float targetHz) {
    if (originalHz <= 0.0f || targetHz <= 0.0f) {
        return 0.0f;
    }
    return 12.0f * std::log2(targetHz / originalHz);
}

CombinedMetrics CombinedBenchmark::runPitchThenStretch(
    const AudioBuffer& input,
    float semitones,
    float durationRatio
) {
    CombinedMetrics metrics;
    metrics.methodName = "Sequential: Pitch then TimeStretch";
    metrics.targetPitchSemitones = semitones;
    metrics.targetDurationRatio = durationRatio;

    metrics.originalPitch = measureAveragePitch(input);
    metrics.originalDuration = measureDuration(input);

    auto startTime = startTimer();

    // 1. Pitch shift
    ExternalPitchShiftStrategy pitchShifter(true, false);
    AudioBuffer afterPitch = pitchShifter.shiftPitch(input, semitones);

    // 2. Time stretch
    ExternalTimeStretchStrategy timeStretcher(true, false);
    metrics.outputAudio = timeStretcher.stretch(afterPitch, durationRatio);

    metrics.processingTimeMs = stopTimer(startTime);

    metrics.outputPitch = measureAveragePitch(metrics.outputAudio);
    metrics.outputDuration = measureDuration(metrics.outputAudio);

    metrics.actualPitchSemitones = hertzToSemitones(metrics.originalPitch, metrics.outputPitch);
    metrics.pitchError = metrics.actualPitchSemitones - semitones;

    metrics.actualDurationRatio = metrics.outputDuration / metrics.originalDuration;
    metrics.durationError = ((metrics.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return metrics;
}

CombinedMetrics CombinedBenchmark::runStretchThenPitch(
    const AudioBuffer& input,
    float semitones,
    float durationRatio
) {
    CombinedMetrics metrics;
    metrics.methodName = "Sequential: TimeStretch then Pitch";
    metrics.targetPitchSemitones = semitones;
    metrics.targetDurationRatio = durationRatio;

    metrics.originalPitch = measureAveragePitch(input);
    metrics.originalDuration = measureDuration(input);

    auto startTime = startTimer();

    // 1. Time stretch
    ExternalTimeStretchStrategy timeStretcher(true, false);
    AudioBuffer afterStretch = timeStretcher.stretch(input, durationRatio);

    // 2. Pitch shift
    ExternalPitchShiftStrategy pitchShifter(true, false);
    metrics.outputAudio = pitchShifter.shiftPitch(afterStretch, semitones);

    metrics.processingTimeMs = stopTimer(startTime);

    metrics.outputPitch = measureAveragePitch(metrics.outputAudio);
    metrics.outputDuration = measureDuration(metrics.outputAudio);

    metrics.actualPitchSemitones = hertzToSemitones(metrics.originalPitch, metrics.outputPitch);
    metrics.pitchError = metrics.actualPitchSemitones - semitones;

    metrics.actualDurationRatio = metrics.outputDuration / metrics.originalDuration;
    metrics.durationError = ((metrics.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return metrics;
}

CombinedMetrics CombinedBenchmark::runSoundTouchDirect(
    const AudioBuffer& input,
    float semitones,
    float durationRatio
) {
    CombinedMetrics metrics;
    metrics.methodName = "Direct: SoundTouch Combined";
    metrics.targetPitchSemitones = semitones;
    metrics.targetDurationRatio = durationRatio;

    metrics.originalPitch = measureAveragePitch(input);
    metrics.originalDuration = measureDuration(input);

    auto startTime = startTimer();

    // SoundTouch에 pitch와 tempo를 동시에 설정
    soundtouch::SoundTouch st;
    st.setSampleRate(input.getSampleRate());
    st.setChannels(input.getChannels());
    st.setPitchSemiTones(semitones);
    st.setTempo(1.0 / durationRatio);  // tempo = 1 / ratio

    const auto& inputData = input.getData();
    st.putSamples(inputData.data(), inputData.size() / input.getChannels());
    st.flush();

    std::vector<float> outputData;
    const int BUFFER_SIZE = 4096;
    std::vector<float> tempBuffer(BUFFER_SIZE * input.getChannels());

    int numSamples;
    while ((numSamples = st.receiveSamples(tempBuffer.data(), BUFFER_SIZE)) > 0) {
        outputData.insert(outputData.end(),
                         tempBuffer.begin(),
                         tempBuffer.begin() + numSamples * input.getChannels());
    }

    metrics.outputAudio = AudioBuffer(input.getSampleRate(), input.getChannels());
    metrics.outputAudio.setData(outputData);

    metrics.processingTimeMs = stopTimer(startTime);

    metrics.outputPitch = measureAveragePitch(metrics.outputAudio);
    metrics.outputDuration = measureDuration(metrics.outputAudio);

    metrics.actualPitchSemitones = hertzToSemitones(metrics.originalPitch, metrics.outputPitch);
    metrics.pitchError = metrics.actualPitchSemitones - semitones;

    metrics.actualDurationRatio = metrics.outputDuration / metrics.originalDuration;
    metrics.durationError = ((metrics.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return metrics;
}

CombinedMetrics CombinedBenchmark::runPhaseVocoderCombined(
    const AudioBuffer& input,
    float semitones,
    float durationRatio
) {
    CombinedMetrics metrics;
    metrics.methodName = "Sequential: Phase Vocoder (Pitch + TimeStretch)";
    metrics.targetPitchSemitones = semitones;
    metrics.targetDurationRatio = durationRatio;

    metrics.originalPitch = measureAveragePitch(input);
    metrics.originalDuration = measureDuration(input);

    auto startTime = startTimer();

    // 1. Pitch shift (Phase Vocoder)
    HighQualityPitchShiftStrategy pitchShifter(1024, 256);
    AudioBuffer afterPitch = pitchShifter.shiftPitch(input, semitones);

    // 2. Time stretch (Phase Vocoder)
    PhaseVocoderTimeStretchStrategy timeStretcher(2048, 512);
    metrics.outputAudio = timeStretcher.stretch(afterPitch, durationRatio);

    metrics.processingTimeMs = stopTimer(startTime);

    metrics.outputPitch = measureAveragePitch(metrics.outputAudio);
    metrics.outputDuration = measureDuration(metrics.outputAudio);

    metrics.actualPitchSemitones = hertzToSemitones(metrics.originalPitch, metrics.outputPitch);
    metrics.pitchError = metrics.actualPitchSemitones - semitones;

    metrics.actualDurationRatio = metrics.outputDuration / metrics.originalDuration;
    metrics.durationError = ((metrics.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return metrics;
}

CombinedMetrics CombinedBenchmark::runRubberBandDirect(
    const AudioBuffer& input,
    float semitones,
    float durationRatio
) {
    CombinedMetrics metrics;
    metrics.methodName = "Direct: RubberBand Combined";
    metrics.targetPitchSemitones = semitones;
    metrics.targetDurationRatio = durationRatio;

    metrics.originalPitch = measureAveragePitch(input);
    metrics.originalDuration = measureDuration(input);

    auto startTime = startTimer();

    // Semitones를 pitch scale로 변환: scale = 2^(semitones/12)
    double pitchScale = std::pow(2.0, semitones / 12.0);

    // RubberBand 설정
    RubberBandOptions options =
        RubberBandOptionProcessOffline |
        RubberBandOptionEngineFiner |
        RubberBandOptionTransientsMixed;

    RubberBandState rubberband = rubberband_new(
        input.getSampleRate(),
        input.getChannels(),
        options,
        durationRatio,  // time ratio
        pitchScale      // pitch scale
    );

    if (!rubberband) {
        // 실패 시 원본 반환
        metrics.outputAudio = input;
        metrics.processingTimeMs = 0.0;
        return metrics;
    }

    const auto& inputData = input.getData();
    int channels = input.getChannels();
    int inputLength = inputData.size() / channels;

    // 예상 입력 길이 설정
    rubberband_set_expected_input_duration(rubberband, inputLength);

    // 입력 데이터를 채널별로 분리
    std::vector<float*> inputChannels(channels);
    std::vector<std::vector<float>> channelData(channels);

    for (int ch = 0; ch < channels; ++ch) {
        channelData[ch].resize(inputLength);
        for (int i = 0; i < inputLength; ++i) {
            channelData[ch][i] = inputData[i * channels + ch];
        }
        inputChannels[ch] = channelData[ch].data();
    }

    // 입력 데이터 처리
    rubberband_process(rubberband, inputChannels.data(), inputLength, 1);

    // 출력 데이터 가져오기
    int outputLength = static_cast<int>(inputLength * durationRatio * pitchScale);
    std::vector<float*> outputChannels(channels);
    std::vector<std::vector<float>> outputChannelData(channels);

    for (int ch = 0; ch < channels; ++ch) {
        outputChannelData[ch].resize(outputLength);
        outputChannels[ch] = outputChannelData[ch].data();
    }

    int available = rubberband_available(rubberband);
    if (available < 0) available = outputLength;

    int retrieved = rubberband_retrieve(rubberband, outputChannels.data(), available);

    // 채널별 데이터를 인터리브 형식으로 변환
    std::vector<float> outputData;
    outputData.reserve(retrieved * channels);

    for (int i = 0; i < retrieved; ++i) {
        for (int ch = 0; ch < channels; ++ch) {
            outputData.push_back(outputChannelData[ch][i]);
        }
    }

    rubberband_delete(rubberband);

    metrics.outputAudio = AudioBuffer(input.getSampleRate(), channels);
    metrics.outputAudio.setData(outputData);

    metrics.processingTimeMs = stopTimer(startTime);

    metrics.outputPitch = measureAveragePitch(metrics.outputAudio);
    metrics.outputDuration = measureDuration(metrics.outputAudio);

    metrics.actualPitchSemitones = hertzToSemitones(metrics.originalPitch, metrics.outputPitch);
    metrics.pitchError = metrics.actualPitchSemitones - semitones;

    metrics.actualDurationRatio = metrics.outputDuration / metrics.originalDuration;
    metrics.durationError = ((metrics.actualDurationRatio - durationRatio) / durationRatio) * 100.0f;

    return metrics;
}

std::vector<CombinedMetrics> CombinedBenchmark::runAllBenchmarks(
    const AudioBuffer& input,
    float semitones,
    float durationRatio
) {
    std::vector<CombinedMetrics> results;

    results.push_back(runPitchThenStretch(input, semitones, durationRatio));
    results.push_back(runStretchThenPitch(input, semitones, durationRatio));
    results.push_back(runSoundTouchDirect(input, semitones, durationRatio));
    results.push_back(runPhaseVocoderCombined(input, semitones, durationRatio));
    results.push_back(runRubberBandDirect(input, semitones, durationRatio));

    return results;
}

std::string CombinedBenchmark::escapeJSON(const std::string& input) {
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

std::string CombinedBenchmark::resultsToJSON(const std::vector<CombinedMetrics>& results) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"benchmarkType\": \"Combined\",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    json << "  \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& m = results[i];
        json << "    {\n";
        json << "      \"method\": \"" << escapeJSON(m.methodName) << "\",\n";
        json << "      \"targetPitchSemitones\": " << m.targetPitchSemitones << ",\n";
        json << "      \"targetDurationRatio\": " << m.targetDurationRatio << ",\n";
        json << "      \"processingTimeMs\": " << m.processingTimeMs << ",\n";
        json << "      \"originalPitch\": " << m.originalPitch << ",\n";
        json << "      \"outputPitch\": " << m.outputPitch << ",\n";
        json << "      \"actualPitchSemitones\": " << m.actualPitchSemitones << ",\n";
        json << "      \"pitchError\": " << m.pitchError << ",\n";
        json << "      \"originalDuration\": " << m.originalDuration << ",\n";
        json << "      \"outputDuration\": " << m.outputDuration << ",\n";
        json << "      \"actualDurationRatio\": " << m.actualDurationRatio << ",\n";
        json << "      \"durationError\": " << m.durationError << "\n";
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

std::string CombinedBenchmark::resultsToHTML(const std::vector<CombinedMetrics>& results) {
    std::ostringstream html;
    html << std::fixed << std::setprecision(2);

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"ko\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>Combined Benchmark Report</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    html << "    .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "    h1 { color: #333; border-bottom: 3px solid #FF9800; padding-bottom: 10px; }\n";
    html << "    h2 { color: #555; margin-top: 30px; }\n";
    html << "    table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
    html << "    th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html << "    th { background-color: #FF9800; color: white; font-weight: bold; }\n";
    html << "    tr:hover { background-color: #f5f5f5; }\n";
    html << "    .summary { background: #fff3e0; padding: 15px; border-radius: 5px; margin: 20px 0; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div class=\"container\">\n";
    html << "    <h1>Combined (Pitch + Duration) Benchmark Report</h1>\n";
    html << "    <p>Generated: " << std::time(nullptr) << "</p>\n";

    html << "    <h2>Results</h2>\n";
    html << "    <table>\n";
    html << "      <tr>\n";
    html << "        <th>Method</th>\n";
    html << "        <th>Processing Time (ms)</th>\n";
    html << "        <th>Pitch Error (semitones)</th>\n";
    html << "        <th>Duration Error (%)</th>\n";
    html << "      </tr>\n";

    for (const auto& m : results) {
        html << "      <tr>\n";
        html << "        <td>" << m.methodName << "</td>\n";
        html << "        <td>" << m.processingTimeMs << "</td>\n";
        html << "        <td>" << std::abs(m.pitchError) << "</td>\n";
        html << "        <td>" << std::abs(m.durationError) << "</td>\n";
        html << "      </tr>\n";
    }

    html << "    </table>\n";

    html << "    <div class=\"summary\">\n";
    html << "      <h2>Summary</h2>\n";
    html << "      <p><strong>Total Methods Tested:</strong> " << results.size() << "</p>\n";
    if (!results.empty()) {
        html << "      <p><strong>Target Pitch Shift:</strong> " << results[0].targetPitchSemitones << " semitones</p>\n";
        html << "      <p><strong>Target Duration Ratio:</strong> " << results[0].targetDurationRatio << "x</p>\n";
    }
    html << "    </div>\n";

    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}
