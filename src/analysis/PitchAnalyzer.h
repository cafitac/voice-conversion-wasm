#ifndef PITCHANALYZER_H
#define PITCHANALYZER_H

#include "../audio/AudioBuffer.h"
#include "../audio/AudioPreprocessor.h"
#include <vector>

struct PitchPoint {
    float time;      // 시간 (초)
    float frequency; // 주파수 (Hz)
    float confidence; // 신뢰도 (0.0 ~ 1.0)
};

// Pitch 추출 결과 (주파수와 신뢰도)
struct PitchResult {
    float frequency;  // 주파수 (Hz), 0.0이면 감지 실패
    float confidence; // 신뢰도 (0.0 ~ 1.0)
};

class PitchAnalyzer {
public:
    PitchAnalyzer();
    ~PitchAnalyzer();

    // Pitch 분석 (전체 오디오) - 기존 방식
    std::vector<PitchPoint> analyze(const AudioBuffer& buffer, float frameSize = 0.02f);

    // Pitch 분석 (전처리된 프레임 사용) - 새로운 방식
    std::vector<PitchPoint> analyzeFrames(const std::vector<FrameData>& frames, int sampleRate);

    // 단일 프레임 Pitch 추출 (Autocorrelation 기반)
    PitchResult extractPitch(const std::vector<float>& frame, int sampleRate, float minFreq = 80.0f, float maxFreq = 400.0f);

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
