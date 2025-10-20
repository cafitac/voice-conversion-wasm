#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include "AudioBuffer.h"

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();

    // 기본 오디오 처리
    static void normalize(AudioBuffer& buffer);
    static void amplify(AudioBuffer& buffer, float gain);

    // 노이즈 제거 (간단한 저역 통과 필터)
    static void applyLowPassFilter(AudioBuffer& buffer, float cutoffFreq);

    // 무음 제거
    static void trimSilence(AudioBuffer& buffer, float threshold = 0.01f);

private:
    // 내부 헬퍼 함수들
    static float calculateRMS(const std::vector<float>& data, size_t start, size_t length);
};

#endif // AUDIOPROCESSOR_H
