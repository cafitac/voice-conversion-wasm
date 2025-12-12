/**
 * SimplePitchShifter.h
 *
 * 피치 변경 알고리즘 헤더 파일
 */

#ifndef SIMPLE_PITCH_SHIFTER_H
#define SIMPLE_PITCH_SHIFTER_H

#include "../audio/AudioBuffer.h"
#include "../performance/PerformanceChecker.h"
#include "SimpleTimeStretcher.h"

class SimplePitchShifter {
public:
    SimplePitchShifter();

    /**
     * 오디오의 피치를 변경 (길이는 유지)
     * @param input 입력 오디오
     * @param semitones 반음 단위 (-12 ~ +12)
     * @param perfChecker 성능 측정 (optional)
     * @return 피치가 변경된 오디오
     */
    AudioBuffer process(const AudioBuffer& input, float semitones, PerformanceChecker* perfChecker = nullptr);

    /**
     * 멀티스레딩을 사용한 병렬 처리 버전
     * @param input 입력 오디오
     * @param semitones 반음 단위 (-12 ~ +12)
     * @param numThreads 사용할 스레드 수 (0 = 자동 감지)
     * @param perfChecker 성능 측정 (optional)
     * @return 피치가 변경된 오디오
     */
    AudioBuffer processParallel(const AudioBuffer& input, float semitones,
                                int numThreads = 0, PerformanceChecker* perfChecker = nullptr);

private:
    SimpleTimeStretcher timeStretcher;

    /**
     * 반음을 비율로 변환
     */
    float semitonesToRatio(float semitones);

    /**
     * 리샘플링
     */
    AudioBuffer resample(const AudioBuffer& input, float ratio);

    /**
     * 선형 보간
     */
    float linearInterpolate(float sample1, float sample2, float fraction);
};

#endif // SIMPLE_PITCH_SHIFTER_H
