#include "DurationAnalyzer.h"
#include <cmath>
#include <algorithm>

DurationAnalyzer::DurationAnalyzer()
    : threshold_(0.02f), minSegmentDuration_(0.1f) {
}

DurationAnalyzer::~DurationAnalyzer() {
}

std::vector<DurationSegment> DurationAnalyzer::analyzeSegments(const AudioBuffer& buffer, float threshold) {
    std::vector<DurationSegment> segments;

    const auto& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int frameLength = static_cast<int>(0.02f * sampleRate); // 20ms frames

    bool inSegment = false;
    size_t segmentStart = 0;

    for (size_t i = 0; i < data.size(); i += frameLength) {
        size_t end = std::min(i + frameLength, data.size());
        float energy = calculateRMS(data, i, end - i);

        if (energy > threshold) {
            if (!inSegment) {
                // 새 세그먼트 시작
                segmentStart = i;
                inSegment = true;
            }
        } else {
            if (inSegment) {
                // 세그먼트 종료
                float startTime = static_cast<float>(segmentStart) / sampleRate;
                float endTime = static_cast<float>(i) / sampleRate;
                float duration = endTime - startTime;

                if (duration >= minSegmentDuration_) {
                    DurationSegment segment;
                    segment.startTime = startTime;
                    segment.endTime = endTime;
                    segment.duration = duration;
                    segment.energy = energy;
                    segments.push_back(segment);
                }
                inSegment = false;
            }
        }
    }

    // 마지막 세그먼트 처리
    if (inSegment) {
        float startTime = static_cast<float>(segmentStart) / sampleRate;
        float endTime = static_cast<float>(data.size()) / sampleRate;
        float duration = endTime - startTime;

        if (duration >= minSegmentDuration_) {
            DurationSegment segment;
            segment.startTime = startTime;
            segment.endTime = endTime;
            segment.duration = duration;
            segment.energy = calculateRMS(data, segmentStart, data.size() - segmentStart);
            segments.push_back(segment);
        }
    }

    return segments;
}

std::vector<float> DurationAnalyzer::analyzeDurationCurve(const AudioBuffer& buffer, float frameSize) {
    std::vector<float> curve;

    const auto& data = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int frameLength = static_cast<int>(frameSize * sampleRate);
    int hopSize = frameLength / 2; // 50% overlap

    for (size_t i = 0; i + frameLength < data.size(); i += hopSize) {
        float energy = calculateRMS(data, i, frameLength);
        curve.push_back(energy);
    }

    return curve;
}

void DurationAnalyzer::setThreshold(float threshold) {
    threshold_ = threshold;
}

void DurationAnalyzer::setMinSegmentDuration(float duration) {
    minSegmentDuration_ = duration;
}

float DurationAnalyzer::calculateEnergy(const std::vector<float>& frame) {
    float energy = 0.0f;
    for (float sample : frame) {
        energy += sample * sample;
    }
    return energy;
}

float DurationAnalyzer::calculateRMS(const std::vector<float>& data, size_t start, size_t length) {
    if (length == 0 || start + length > data.size()) return 0.0f;

    float sum = 0.0f;
    for (size_t i = start; i < start + length; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / length);
}
