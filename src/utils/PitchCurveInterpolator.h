#ifndef PITCHCURVEINTERPOLATOR_H
#define PITCHCURVEINTERPOLATOR_H

#include <vector>
#include <cmath>

/**
 * PitchEditPoint
 *
 * Pitch 편집 포인트 구조체
 * JavaScript에서 전달되는 편집 데이터
 */
struct PitchEditPoint {
    float time;       // 초 단위 시간
    float semitones;  // Pitch shift 양 (semitones)

    PitchEditPoint() : time(0.0f), semitones(0.0f) {}
    PitchEditPoint(float t, float s) : time(t), semitones(s) {}
};

/**
 * PitchCurveInterpolator
 *
 * Pitch curve 보간 유틸리티 클래스
 * Cubic spline 보간을 사용하여 부드러운 pitch 변화 곡선 생성
 *
 * 사용 흐름:
 * 1. JavaScript에서 편집 포인트들 전달
 * 2. interpolatePitchCurve()로 전체 오디오에 대한 pitch curve 생성
 * 3. 생성된 curve를 AudioBuffer에 저장
 * 4. Strategy가 이 curve를 참조하여 variable pitch shift 적용
 */
class PitchCurveInterpolator {
public:
    /**
     * Pitch curve 생성 (Cubic spline 보간)
     *
     * @param editPoints 편집 포인트 배열 (시간순 정렬 필요)
     * @param totalSamples 전체 오디오 샘플 수
     * @param sampleRate 샘플 레이트 (Hz)
     * @return 각 샘플의 semitones 값 배열
     *
     * 동작:
     * - 편집 포인트가 없는 구간: semitones = 0 (원본 유지)
     * - 편집 포인트 사이: Cubic spline으로 부드럽게 보간
     * - 첫/마지막 포인트 이전/이후: semitones = 0
     */
    static std::vector<float> interpolatePitchCurve(
        const std::vector<PitchEditPoint>& editPoints,
        int totalSamples,
        int sampleRate
    );

    /**
     * 특정 시간의 semitones 계산 (Cubic spline 보간)
     *
     * @param time 시간 (초)
     * @param editPoints 편집 포인트 배열
     * @return 보간된 semitones 값
     *
     * interpolatePitchCurve와 동일한 알고리즘 사용
     * 한 샘플만 필요한 경우 사용
     */
    static float getSemitonesAtTime(
        float time,
        const std::vector<PitchEditPoint>& editPoints
    );

private:
    /**
     * Cubic spline 계수 계산
     *
     * Natural cubic spline을 사용
     * - 첫/마지막 포인트에서 2차 미분 = 0
     * - 연속성 보장: 값, 1차 미분, 2차 미분 모두 연속
     */
    static void calculateCubicSplineCoefficients(
        const std::vector<float>& x,  // 시간
        const std::vector<float>& y,  // semitones
        std::vector<float>& a,        // 계수 a
        std::vector<float>& b,        // 계수 b
        std::vector<float>& c,        // 계수 c
        std::vector<float>& d         // 계수 d
    );

    /**
     * Cubic spline 평가
     *
     * 주어진 x 값에 대해 spline 값 계산
     * f(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
     */
    static float evaluateCubicSpline(
        float x,
        const std::vector<float>& xPoints,
        const std::vector<float>& a,
        const std::vector<float>& b,
        const std::vector<float>& c,
        const std::vector<float>& d
    );

    /**
     * Tridiagonal matrix solver (Thomas algorithm)
     *
     * Cubic spline 계수 계산에 사용
     * Ax = d 형태의 삼중대각 시스템 해결
     */
    static std::vector<float> solveTridiagonal(
        const std::vector<float>& a,  // 하부 대각선
        const std::vector<float>& b,  // 주 대각선
        const std::vector<float>& c,  // 상부 대각선
        const std::vector<float>& d   // 우변
    );
};

#endif // PITCHCURVEINTERPOLATOR_H
