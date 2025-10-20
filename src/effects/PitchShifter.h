#ifndef PITCHSHIFTER_H
#define PITCHSHIFTER_H

#include "../audio/AudioBuffer.h"

class PitchShifter {
public:
    PitchShifter();
    ~PitchShifter();

    // Pitch 변환 (semitones: 반음 단위, 예: 12 = 1 옥타브 위)
    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones);

    // Pitch 변환 (ratio: 비율, 예: 2.0 = 2배 높은 주파수)
    AudioBuffer shiftPitchByRatio(const AudioBuffer& input, float ratio);

    // 시간별로 다른 pitch 적용 (그래프 조절용)
    AudioBuffer shiftPitchCurve(const AudioBuffer& input, const std::vector<float>& pitchCurve);

private:
    // 간단한 리샘플링 기반 pitch shift
    std::vector<float> resample(const std::vector<float>& input, float ratio);

    // Linear interpolation
    float interpolate(const std::vector<float>& data, float position);
};

#endif // PITCHSHIFTER_H
