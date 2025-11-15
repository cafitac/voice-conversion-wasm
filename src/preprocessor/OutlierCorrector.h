#ifndef OUTLIERCORRECTOR_H
#define OUTLIERCORRECTOR_H

#include "IFramePreprocessor.h"

/**
 * OutlierCorrector
 *
 * Gradient 기반 극단값 보정기
 *
 * 동작 원리:
 * 1. 각 프레임의 앞뒤 프레임과의 변화율(gradient) 계산
 * 2. 변화율이 threshold를 초과하면 outlier로 판단
 * 3. 주변 프레임의 weighted average로 보정
 * 4. Multi-pass로 연속된 outlier 처리
 *
 * 예시:
 *   입력: [+2, +3, +15, +2, +3]
 *   변화율: +1, +12, -13, +1  ← +12, -13은 threshold(3) 초과
 *   보정: [+2, +3, +2.5, +2, +3]  ← (+3 + +2) / 2 = +2.5
 *
 * 장점:
 * - 절대값이 아닌 연속성 기반 판단
 * - 자연스러운 큰 변화는 보정하지 않음
 * - 사용자가 극단적인 편집을 해도 자연스러운 결과
 */
class OutlierCorrector : public IFramePreprocessor {
public:
    /**
     * @param gradientThreshold 허용 가능한 최대 변화율 (semitones, 기본 3.0)
     * @param windowSize 주변 프레임 확인 범위 (기본 2)
     * @param maxIterations 최대 반복 횟수 (연속된 outlier 처리용, 기본 3)
     */
    OutlierCorrector(
        float gradientThreshold = 3.0f,
        int windowSize = 2,
        int maxIterations = 3
    );

    ~OutlierCorrector() override;

    std::vector<FrameData> process(const std::vector<FrameData>& frames) override;

    const char* getName() const override {
        return "OutlierCorrector (Gradient-based)";
    }

    /**
     * Gradient threshold 설정
     */
    void setGradientThreshold(float threshold);

    /**
     * Window size 설정
     */
    void setWindowSize(int size);

private:
    float gradientThreshold_;
    int windowSize_;
    int maxIterations_;

    /**
     * 특정 프레임이 outlier인지 판단
     *
     * @param frames 전체 프레임들
     * @param index 검사할 프레임 인덱스
     * @return true if outlier
     */
    bool isOutlier(const std::vector<FrameData>& frames, int index) const;

    /**
     * Outlier 보정 값 계산
     *
     * Weighted average 사용:
     * - 가까운 프레임일수록 가중치 높음
     * - weight = 1.0 / distance
     *
     * @param frames 전체 프레임들
     * @param index 보정할 프레임 인덱스
     * @return 보정된 pitchSemitones 값
     */
    float correctValue(const std::vector<FrameData>& frames, int index) const;

    /**
     * Gradient (변화율) 계산
     *
     * @param value1 이전 값
     * @param value2 현재 값
     * @return 절대 변화율
     */
    float calculateGradient(float value1, float value2) const;

    /**
     * 최대 gradient 계산
     *
     * windowSize 범위 내의 모든 프레임과의 gradient 중 최대값
     *
     * @param frames 전체 프레임들
     * @param index 검사할 프레임 인덱스
     * @return 최대 gradient
     */
    float getMaxGradient(const std::vector<FrameData>& frames, int index) const;
};

#endif // OUTLIERCORRECTOR_H
