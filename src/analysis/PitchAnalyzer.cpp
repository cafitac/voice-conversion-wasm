#include "PitchAnalyzer.h"
#include <algorithm>

using namespace std;

PitchAnalyzer::PitchAnalyzer()
    : minFreq_(80.0f), maxFreq_(400.0f) {
}

PitchAnalyzer::~PitchAnalyzer() {
}

vector<PitchPoint> PitchAnalyzer::analyze(const AudioBuffer& buffer, float frameSize) {
    vector<PitchPoint> pitchPoints;

    const auto& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int frameLength = static_cast<int>(frameSize * sampleRate);
    int hopSize = frameLength / 2; // 50% overlap

    // 예상 포인트 개수만큼 메모리 미리 확보
    size_t estimatedPoints = (data.size() - frameLength) / hopSize + 1;
    pitchPoints.reserve(estimatedPoints);

    for (size_t i = 0; i + frameLength < data.size(); i += hopSize) {
        vector<float> frame(data.begin() + i, data.begin() + i + frameLength);

        PitchResult result = extractPitch(frame, sampleRate, minFreq_, maxFreq_);

        if (result.frequency > 0.0f) {
            PitchPoint point;
            point.time = static_cast<float>(i) / sampleRate;
            point.frequency = result.frequency;
            point.confidence = result.confidence;  // 실제 계산된 신뢰도 사용
            pitchPoints.push_back(point);
        }
    }

    // Median filter 적용하여 튀는 값 제거
    return applyMedianFilter(pitchPoints, 5);
}

vector<PitchPoint> PitchAnalyzer::analyzeFrames(const vector<FrameData>& frames, int sampleRate) {
    vector<PitchPoint> pitchPoints;
    pitchPoints.reserve(frames.size()); // 최대 프레임 개수만큼 확보

    for (const auto& frame : frames) {
        // VAD 체크: 음성 구간만 분석
        if (!frame.isVoice) {
            continue;
        }

        PitchResult result = extractPitch(frame.samples, sampleRate, minFreq_, maxFreq_);

        if (result.frequency > 0.0f) {
            PitchPoint point;
            point.time = frame.time;
            point.frequency = result.frequency;
            point.confidence = result.confidence;
            pitchPoints.push_back(point);
        }
    }

    // Median filter 적용하여 튀는 값 제거
    return applyMedianFilter(pitchPoints, 5);
}

PitchResult PitchAnalyzer::extractPitch(const vector<float>& frame, int sampleRate, float minFreq, float maxFreq) {
    PitchResult result;
    result.frequency = 0.0f;
    result.confidence = 0.0f;

    if (frame.empty()) return result;

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

    // Confidence 계산: autocorrelation 피크 값 사용
    // autocorr은 이미 정규화되어 있으므로 0~1 범위
    result.confidence = maxValue;

    // Parabolic interpolation으로 정확한 피크 위치 찾기
    float refinedLag = findPeakParabolic(autocorr, peakLag);

    // 주파수 계산
    if (refinedLag > 0) {
        result.frequency = static_cast<float>(sampleRate) / refinedLag;
    }

    return result;
}

void PitchAnalyzer::setMinFrequency(float freq) {
    minFreq_ = freq;
}

void PitchAnalyzer::setMaxFrequency(float freq) {
    maxFreq_ = freq;
}

vector<float> PitchAnalyzer::calculateAutocorrelation(const vector<float>& signal) {
    size_t n = signal.size();
    vector<float> autocorr(n, 0.0f);

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

float PitchAnalyzer::findPeakParabolic(const vector<float>& data, int index) {
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

vector<PitchPoint> PitchAnalyzer::applyMedianFilter(const vector<PitchPoint>& points, int windowSize) {
    if (points.size() < static_cast<size_t>(windowSize)) {
        return points;
    }

    vector<PitchPoint> filtered;
    filtered.reserve(points.size()); // 결과 크기는 입력과 동일
    int halfWindow = windowSize / 2;

    for (size_t i = 0; i < points.size(); ++i) {
        // 윈도우 범위 계산
        int start = std::max(0, static_cast<int>(i) - halfWindow);
        int end = std::min(static_cast<int>(points.size()) - 1, static_cast<int>(i) + halfWindow);

        // 윈도우 내 frequency 값들 수집
        vector<float> windowFreqs;
        windowFreqs.reserve(end - start + 1); // 윈도우 크기만큼 확보
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
