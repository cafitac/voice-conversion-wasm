#ifndef TIMESCALEMODIFIER_H
#define TIMESCALEMODIFIER_H

#include <vector>

/**
 * TimeScaleModifier
 *
 * Duration 수정을 위한 시간 스케일 조정 로직
 * 프레임의 synthesis hop size를 조절하여 시간을 늘리거나 줄임
 */
class TimeScaleModifier {
public:
    TimeScaleModifier();
    ~TimeScaleModifier();

    /**
     * 균일한 time ratio 벡터 생성
     *
     * @param numFrames 프레임 개수
     * @param ratio 시간 스케일 비율 (1.0 = 원본, 1.2 = 20% 늘림, 0.9 = 10% 줄임)
     * @return 각 프레임의 time ratio 벡터
     */
    std::vector<float> createUniformTimeRatios(size_t numFrames, float ratio);

    /**
     * 예상 출력 길이 계산
     *
     * @param originalDuration 원본 길이 (초)
     * @param ratio 시간 스케일 비율
     * @return 예상 출력 길이 (초)
     */
    float calculateExpectedDuration(float originalDuration, float ratio);

    /**
     * 프레임별 time ratio 벡터 생성 (가변 속도)
     * 추후 프레임별로 다른 속도를 적용하고 싶을 때 사용
     *
     * @param numFrames 프레임 개수
     * @param ratios 각 프레임별 비율 (크기가 numFrames보다 작으면 마지막 값 재사용)
     * @return 각 프레임의 time ratio 벡터
     */
    std::vector<float> createVariableTimeRatios(size_t numFrames, const std::vector<float>& ratios);
};

#endif // TIMESCALEMODIFIER_H
