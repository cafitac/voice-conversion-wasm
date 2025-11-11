#ifndef POWERANALYZER_H
#define POWERANALYZER_H

#include "../audio/AudioBuffer.h"
#include "../audio/AudioPreprocessor.h"
#include <vector>
#include <cmath>

struct PowerPoint {
    float time;      // 시간 (초)
    float rms;       // RMS (Root Mean Square, 0.0 ~ 1.0)
    float dbFS;      // dBFS (Decibels relative to Full Scale, -∞ ~ 0.0)
};

class PowerAnalyzer {
public:
    PowerAnalyzer();
    ~PowerAnalyzer();

    // 전체 오디오의 파워 분석 (시간별 RMS 및 dBFS) - 기존 방식
    std::vector<PowerPoint> analyze(const AudioBuffer& buffer, float frameSize = 0.02f);

    // 전체 오디오의 파워 분석 (전처리된 프레임 사용) - 새로운 방식
    std::vector<PowerPoint> analyzeFrames(const std::vector<FrameData>& frames);

    // 특정 구간의 평균 파워 계산
    PowerPoint analyzeSegment(const AudioBuffer& buffer, float startTime, float endTime);

    // RMS를 dBFS로 변환
    static float rmsTodBFS(float rms);

    // dBFS를 RMS로 변환
    static float dBFSToRms(float dbFS);

private:
    // RMS 계산 (Root Mean Square)
    float calculateRMS(const std::vector<float>& data, size_t start, size_t length);
};

#endif // POWERANALYZER_H
