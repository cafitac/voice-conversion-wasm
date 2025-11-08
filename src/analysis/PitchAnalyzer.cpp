#include "PitchAnalyzer.h"
#include <cmath>
#include <algorithm>

PitchAnalyzer::PitchAnalyzer()
    : minFreq_(80.0f), maxFreq_(400.0f) {
}

PitchAnalyzer::~PitchAnalyzer() {
}

std::vector<PitchPoint> PitchAnalyzer::analyze(const AudioBuffer& buffer, float frameSize) {
    std::vector<PitchPoint> pitchPoints;

    const auto& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int frameLength = static_cast<int>(frameSize * sampleRate);
    int hopSize = frameLength / 2; // 50% overlap

    for (size_t i = 0; i + frameLength < data.size(); i += hopSize) {
        std::vector<float> frame(data.begin() + i, data.begin() + i + frameLength);

        float pitch = extractPitch(frame, sampleRate, minFreq_, maxFreq_);

        if (pitch > 0.0f) {
            PitchPoint point;
            point.time = static_cast<float>(i) / sampleRate;
            point.frequency = pitch;
            point.confidence = 0.8f; // 간단한 구현이므로 고정값
            pitchPoints.push_back(point);
        }
    }

    // Median filter 적용하여 튀는 값 제거
    return applyMedianFilter(pitchPoints, 5);
}

float PitchAnalyzer::extractPitch(const std::vector<float>& frame, int sampleRate, float minFreq, float maxFreq) {
    if (frame.empty()) return 0.0f;

    // Autocorrelation 계산
    auto autocorr = calculateAutocorrelation(frame);

    // 탐색 범위 계산
    int minLag = static_cast<int>(sampleRate / maxFreq);
    int maxLag = static_cast<int>(sampleRate / minFreq);

    if (maxLag >= static_cast<int>(autocorr.size())) {
        maxLag = static_cast<int>(autocorr.size()) - 1;
    }

    // 최대 피크 찾기
    int peakLag = minLag;
    float maxValue = autocorr[minLag];

    for (int lag = minLag; lag <= maxLag; ++lag) {
        if (autocorr[lag] > maxValue) {
            maxValue = autocorr[lag];
            peakLag = lag;
        }
    }

    // Parabolic interpolation으로 정확한 피크 위치 찾기
    float refinedLag = findPeakParabolic(autocorr, peakLag);

    // 주파수 계산
    if (refinedLag > 0) {
        return static_cast<float>(sampleRate) / refinedLag;
    }

    return 0.0f;
}

void PitchAnalyzer::setMinFrequency(float freq) {
    minFreq_ = freq;
}

void PitchAnalyzer::setMaxFrequency(float freq) {
    maxFreq_ = freq;
}

std::vector<float> PitchAnalyzer::calculateAutocorrelation(const std::vector<float>& signal) {
    size_t n = signal.size();
    std::vector<float> autocorr(n, 0.0f);

    for (size_t lag = 0; lag < n; ++lag) {
        float sum = 0.0f;
        for (size_t i = 0; i < n - lag; ++i) {
            sum += signal[i] * signal[i + lag];
        }
        autocorr[lag] = sum;
    }

    // 정규화
    if (autocorr[0] > 0.0f) {
        float norm = autocorr[0];
        for (float& val : autocorr) {
            val /= norm;
        }
    }

    return autocorr;
}

float PitchAnalyzer::findPeakParabolic(const std::vector<float>& data, int index) {
    if (index <= 0 || index >= static_cast<int>(data.size()) - 1) {
        return static_cast<float>(index);
    }

    float alpha = data[index - 1];
    float beta = data[index];
    float gamma = data[index + 1];

    // Parabolic interpolation
    float offset = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);

    return static_cast<float>(index) + offset;
}

std::vector<PitchPoint> PitchAnalyzer::applyMedianFilter(const std::vector<PitchPoint>& points, int windowSize) {
    if (points.size() < static_cast<size_t>(windowSize)) {
        return points;
    }

    std::vector<PitchPoint> filtered;
    int halfWindow = windowSize / 2;

    for (size_t i = 0; i < points.size(); ++i) {
        // 윈도우 범위 계산
        int start = std::max(0, static_cast<int>(i) - halfWindow);
        int end = std::min(static_cast<int>(points.size()) - 1, static_cast<int>(i) + halfWindow);

        // 윈도우 내 frequency 값들 수집
        std::vector<float> windowFreqs;
        for (int j = start; j <= end; ++j) {
            windowFreqs.push_back(points[j].frequency);
        }

        // Median 계산
        std::sort(windowFreqs.begin(), windowFreqs.end());
        float median = windowFreqs[windowFreqs.size() / 2];

        // 필터링된 포인트 생성
        PitchPoint filteredPoint = points[i];
        filteredPoint.frequency = median;
        filtered.push_back(filteredPoint);
    }

    return filtered;
}
