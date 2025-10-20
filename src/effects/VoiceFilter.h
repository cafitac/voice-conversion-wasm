#ifndef VOICEFILTER_H
#define VOICEFILTER_H

#include "../audio/AudioBuffer.h"

enum class FilterType {
    LOW_PASS,
    HIGH_PASS,
    BAND_PASS,
    ROBOT,
    ECHO,
    REVERB
};

class VoiceFilter {
public:
    VoiceFilter();
    ~VoiceFilter();

    // 음성 필터 적용
    AudioBuffer applyFilter(const AudioBuffer& input, FilterType type, float param1 = 0.5f, float param2 = 0.5f);

    // 개별 효과
    AudioBuffer applyLowPass(const AudioBuffer& input, float cutoff);
    AudioBuffer applyHighPass(const AudioBuffer& input, float cutoff);
    AudioBuffer applyBandPass(const AudioBuffer& input, float lowCutoff, float highCutoff);
    AudioBuffer applyRobot(const AudioBuffer& input);
    AudioBuffer applyEcho(const AudioBuffer& input, float delay, float feedback);
    AudioBuffer applyReverb(const AudioBuffer& input, float roomSize, float damping);

private:
    // 간단한 필터 구현
    void applySimpleLowPass(std::vector<float>& data, float cutoff, int sampleRate);
    void applySimpleHighPass(std::vector<float>& data, float cutoff, int sampleRate);
};

#endif // VOICEFILTER_H
