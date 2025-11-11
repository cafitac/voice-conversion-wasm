#ifndef DURATIONANALYZER_H
#define DURATIONANALYZER_H

#include "../audio/AudioBuffer.h"
#include "../audio/AudioPreprocessor.h"
#include <vector>

struct DurationSegment {
    float startTime;  // 시작 시간 (초)
    float endTime;    // 끝 시간 (초)
    float duration;   // 지속 시간 (초)
    float energy;     // RMS 에너지 레벨 (호환성 유지, PowerAnalyzer로 계산)
};

class DurationAnalyzer {
public:
    DurationAnalyzer();
    ~DurationAnalyzer();

    // 음성 세그먼트 분석 (무음 기반 분할) - 기존 방식
    std::vector<DurationSegment> analyzeSegments(const AudioBuffer& buffer, float threshold = 0.02f);

    // 음성 세그먼트 분석 (전처리된 프레임 사용) - 새로운 방식
    std::vector<DurationSegment> analyzeFrames(const std::vector<FrameData>& frames);

    // 전체 오디오의 지속 시간 정보
    std::vector<float> analyzeDurationCurve(const AudioBuffer& buffer, float frameSize = 0.02f);

    // 설정
    void setThreshold(float threshold);
    void setMinSegmentDuration(float duration);

private:
    float threshold_;
    float minSegmentDuration_;

    // RMS 계산 (threshold 비교용)
    float calculateRMS(const std::vector<float>& data, size_t start, size_t length);
};

#endif // DURATIONANALYZER_H
