#ifndef PITCHANALYZER_H
#define PITCHANALYZER_H

#include "../audio/AudioBuffer.h"
#include <vector>

struct PitchPoint {
    float time;      // 시간 (초)
    float frequency; // 주파수 (Hz)
    float confidence; // 신뢰도 (0.0 ~ 1.0)
};

class PitchAnalyzer {
public:
    PitchAnalyzer();
    ~PitchAnalyzer();

    // Pitch 분석 (전체 오디오)
    std::vector<PitchPoint> analyze(const AudioBuffer& buffer, float frameSize = 0.02f);

    // 단일 프레임 Pitch 추출 (Autocorrelation 기반)
    float extractPitch(const std::vector<float>& frame, int sampleRate, float minFreq = 80.0f, float maxFreq = 400.0f);

    // 설정
    void setMinFrequency(float freq);
    void setMaxFrequency(float freq);

private:
    float minFreq_;
    float maxFreq_;

    // Autocorrelation 계산
    std::vector<float> calculateAutocorrelation(const std::vector<float>& signal);

    // Parabolic interpolation으로 정확한 피크 찾기
    float findPeakParabolic(const std::vector<float>& data, int index);

    // Median filter 적용
    std::vector<PitchPoint> applyMedianFilter(const std::vector<PitchPoint>& points, int windowSize = 5);
};

#endif // PITCHANALYZER_H
