#include "PowerAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <limits>

PowerAnalyzer::PowerAnalyzer() {
}

PowerAnalyzer::~PowerAnalyzer() {
}

std::vector<PowerPoint> PowerAnalyzer::analyze(const AudioBuffer& buffer, float frameSize) {
    std::vector<PowerPoint> result;

    const std::vector<float>& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int channels = buffer.getChannels();

    if (data.empty() || sampleRate <= 0 || channels <= 0) {
        return result;
    }

    // 프레임 크기 (샘플 단위)
    size_t frameSamples = static_cast<size_t>(frameSize * sampleRate * channels);

    // 최소 프레임 크기 보장
    if (frameSamples == 0) {
        frameSamples = channels;
    }

    // 프레임별로 RMS 계산
    for (size_t i = 0; i < data.size(); i += frameSamples) {
        size_t length = std::min(frameSamples, data.size() - i);

        PowerPoint point;
        point.time = static_cast<float>(i) / (sampleRate * channels);
        point.rms = calculateRMS(data, i, length);
        point.dbFS = rmsTodBFS(point.rms);

        result.push_back(point);
    }

    return result;
}

std::vector<PowerPoint> PowerAnalyzer::analyzeFrames(const std::vector<FrameData>& frames) {
    std::vector<PowerPoint> result;

    for (const auto& frame : frames) {
        PowerPoint point;
        point.time = frame.time;
        point.rms = frame.rms;  // 전처리된 RMS 사용
        point.dbFS = rmsTodBFS(frame.rms);

        result.push_back(point);
    }

    return result;
}

PowerPoint PowerAnalyzer::analyzeSegment(const AudioBuffer& buffer, float startTime, float endTime) {
    const std::vector<float>& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int channels = buffer.getChannels();

    PowerPoint point;
    point.time = (startTime + endTime) / 2.0f;

    if (data.empty() || sampleRate <= 0 || channels <= 0 || startTime >= endTime) {
        point.rms = 0.0f;
        point.dbFS = -std::numeric_limits<float>::infinity();
        return point;
    }

    // 시작/끝 샘플 인덱스 계산
    size_t startSample = static_cast<size_t>(startTime * sampleRate * channels);
    size_t endSample = static_cast<size_t>(endTime * sampleRate * channels);

    // 범위 체크
    startSample = std::min(startSample, data.size());
    endSample = std::min(endSample, data.size());

    if (startSample >= endSample) {
        point.rms = 0.0f;
        point.dbFS = -std::numeric_limits<float>::infinity();
        return point;
    }

    // RMS 계산
    size_t length = endSample - startSample;
    point.rms = calculateRMS(data, startSample, length);
    point.dbFS = rmsTodBFS(point.rms);

    return point;
}

float PowerAnalyzer::rmsTodBFS(float rms) {
    if (rms <= 0.0f) {
        return -std::numeric_limits<float>::infinity();
    }

    // dBFS = 20 * log10(rms / 1.0)
    // Full Scale = 1.0이므로 분모는 생략
    return 20.0f * std::log10(rms);
}

float PowerAnalyzer::dBFSToRms(float dbFS) {
    if (dbFS == -std::numeric_limits<float>::infinity()) {
        return 0.0f;
    }

    // rms = 10^(dBFS / 20)
    return std::pow(10.0f, dbFS / 20.0f);
}

float PowerAnalyzer::calculateRMS(const std::vector<float>& data, size_t start, size_t length) {
    if (length == 0 || start >= data.size()) {
        return 0.0f;
    }

    // 실제 계산 가능한 길이
    size_t actualLength = std::min(length, data.size() - start);

    // RMS = sqrt(mean(x^2))
    double sumSquares = 0.0;
    for (size_t i = start; i < start + actualLength; ++i) {
        double sample = data[i];
        sumSquares += sample * sample;
    }

    double meanSquare = sumSquares / actualLength;
    return static_cast<float>(std::sqrt(meanSquare));
}
