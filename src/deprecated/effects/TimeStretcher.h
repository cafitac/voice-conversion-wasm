#ifndef TIMESTRETCHER_H
#define TIMESTRETCHER_H

#include "../audio/AudioBuffer.h"

class TimeStretcher {
public:
    TimeStretcher();
    ~TimeStretcher();

    // 시간 늘리기/줄이기 (ratio > 1.0: 느리게, ratio < 1.0: 빠르게)
    AudioBuffer stretch(const AudioBuffer& input, float ratio);

    // 시간별로 다른 stretch 적용 (그래프 조절용)
    AudioBuffer stretchCurve(const AudioBuffer& input, const std::vector<float>& stretchCurve);

private:
    // WSOLA (Waveform Similarity Overlap-Add) 간단 구현
    std::vector<float> wsolaStretch(const std::vector<float>& input, float ratio, int sampleRate);

    // 최적 오버랩 위치 찾기
    int findBestMatch(const std::vector<float>& target, const std::vector<float>& search, int searchRange);

    // 크로스페이드
    void crossfade(std::vector<float>& output, const std::vector<float>& frame, int position, int overlapSize);
};

#endif // TIMESTRETCHER_H
