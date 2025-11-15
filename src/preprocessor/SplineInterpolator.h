#ifndef SPLINEINTERPOLATOR_H
#define SPLINEINTERPOLATOR_H

#include "IFramePreprocessor.h"

/**
 * SplineInterpolator
 *
 * Cubic Spline 보간기
 *
 * 동작 원리:
 * 1. 편집된 프레임들 (isEdited = true)을 보간 포인트로 사용
 * 2. Cubic spline으로 중간 프레임들의 pitchSemitones 계산
 * 3. 보간된 프레임들은 isInterpolated = true로 표시
 *
 * 예시:
 *   입력 (편집된 포인트만):
 *     Frame[0]: time=1.0, pitchSemitones=+2, isEdited=true
 *     Frame[1]: time=3.0, pitchSemitones=+4, isEdited=true
 *
 *   출력 (보간된 프레임 포함):
 *     Frame[0]: time=1.0, pitchSemitones=+2.0, isEdited=true
 *     Frame[1]: time=1.5, pitchSemitones=+2.6, isInterpolated=true
 *     Frame[2]: time=2.0, pitchSemitones=+3.1, isInterpolated=true
 *     Frame[3]: time=2.5, pitchSemitones=+3.5, isInterpolated=true
 *     Frame[4]: time=3.0, pitchSemitones=+4.0, isEdited=true
 *
 * Natural Cubic Spline 사용:
 * - 2차 미분 = 0 at endpoints
 * - C2 연속성 보장
 */
class SplineInterpolator : public IFramePreprocessor {
public:
    /**
     * @param frameInterval 프레임 간격 (초, 기본 0.02 = 20ms)
     * @param sampleRate 샘플 레이트 (Hz, 기본 48000)
     */
    SplineInterpolator(
        float frameInterval = 0.02f,
        int sampleRate = 48000
    );

    ~SplineInterpolator() override;

    std::vector<FrameData> process(const std::vector<FrameData>& frames) override;

    const char* getName() const override {
        return "SplineInterpolator (Cubic Spline)";
    }

    /**
     * Frame interval 설정
     */
    void setFrameInterval(float interval);

    /**
     * Sample rate 설정
     */
    void setSampleRate(int rate);

    /**
     * Total duration 설정 (오디오 전체 길이)
     */
    void setTotalDuration(float duration);

private:
    float frameInterval_;
    int sampleRate_;
    float totalDuration_;

    /**
     * Cubic spline 계수 계산
     *
     * Natural cubic spline을 사용
     * - 첫/마지막 포인트에서 2차 미분 = 0
     * - 연속성 보장: 값, 1차 미분, 2차 미분 모두 연속
     */
    void calculateCubicSplineCoefficients(
        const std::vector<float>& x,  // 시간
        const std::vector<float>& y,  // pitchSemitones
        std::vector<float>& a,        // 계수 a
        std::vector<float>& b,        // 계수 b
        std::vector<float>& c,        // 계수 c
        std::vector<float>& d         // 계수 d
    ) const;

    /**
     * Cubic spline 평가
     *
     * 주어진 x 값에 대해 spline 값 계산
     * f(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
     */
    float evaluateCubicSpline(
        float x,
        const std::vector<float>& xPoints,
        const std::vector<float>& a,
        const std::vector<float>& b,
        const std::vector<float>& c,
        const std::vector<float>& d
    ) const;

    /**
     * Tridiagonal matrix solver (Thomas algorithm)
     *
     * Cubic spline 계수 계산에 사용
     * Ax = d 형태의 삼중대각 시스템 해결
     */
    std::vector<float> solveTridiagonal(
        const std::vector<float>& a,  // 하부 대각선
        const std::vector<float>& b,  // 주 대각선
        const std::vector<float>& c,  // 상부 대각선
        const std::vector<float>& d   // 우변
    ) const;
};

#endif // SPLINEINTERPOLATOR_H
