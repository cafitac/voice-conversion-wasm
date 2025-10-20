#ifndef DURATIONANALYZER_H
#define DURATIONANALYZER_H

#include "../audio/AudioBuffer.h"
#include <vector>

struct DurationSegment {
    float startTime;  // 시작 시간 (초)
    float endTime;    // 끝 시간 (초)
    float duration;   // 지속 시간 (초)
    float energy;     // 에너지 레벨
};

class DurationAnalyzer {
public:
    DurationAnalyzer();
    ~DurationAnalyzer();

    // 음성 세그먼트 분석 (무음 기반 분할)
    std::vector<DurationSegment> analyzeSegments(const AudioBuffer& buffer, float threshold = 0.02f);

    // 전체 오디오의 지속 시간 정보
    std::vector<float> analyzeDurationCurve(const AudioBuffer& buffer, float frameSize = 0.02f);

    // 설정
    void setThreshold(float threshold);
    void setMinSegmentDuration(float duration);

private:
    float threshold_;
    float minSegmentDuration_;

    // 에너지 계산
    float calculateEnergy(const std::vector<float>& frame);

    // RMS 계산
    float calculateRMS(const std::vector<float>& data, size_t start, size_t length);
};

#endif // DURATIONANALYZER_H
