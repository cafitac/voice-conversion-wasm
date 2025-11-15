#include "SplineInterpolator.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

SplineInterpolator::SplineInterpolator(float frameInterval, int sampleRate)
    : frameInterval_(frameInterval), sampleRate_(sampleRate), totalDuration_(0.0f) {
}

SplineInterpolator::~SplineInterpolator() {
}

std::vector<FrameData> SplineInterpolator::process(const std::vector<FrameData>& frames) {
    if (frames.empty()) {
        return frames;
    }

    // 편집된 포인트들만 추출
    std::vector<FrameData> editedFrames;
    for (const auto& frame : frames) {
        if (frame.isEdited) {
            editedFrames.push_back(frame);
        }
    }

    // 편집 포인트가 1개 이하면 보간 없이 전체 프레임 생성
    if (editedFrames.size() < 2) {
        std::vector<FrameData> result;
        float maxTime = (totalDuration_ > 0.0f) ? totalDuration_ : (editedFrames.empty() ? 0.0f : editedFrames.back().time + frameInterval_);

        if (maxTime <= 0.0f) {
            return frames;
        }

        // 단일 편집 포인트: 해당 포인트 이전/이후는 0
        float editTime = editedFrames.empty() ? -1.0f : editedFrames.front().time;
        float editSemitones = editedFrames.empty() ? 0.0f : editedFrames.front().pitchSemitones;

        for (float t = 0.0f; t < maxTime; t += frameInterval_) {
            FrameData frame;
            frame.time = t;
            frame.durationRatio = 1.0f;

            if (editedFrames.empty() || std::abs(t - editTime) > frameInterval_ * 2.0f) {
                frame.pitchSemitones = 0.0f;
            } else {
                frame.pitchSemitones = editSemitones;
                if (std::abs(t - editTime) < frameInterval_ / 2.0f) {
                    frame.isEdited = true;
                }
            }

            result.push_back(frame);
        }

        return result;
    }

    // 편집 포인트를 시간순으로 정렬
    std::sort(editedFrames.begin(), editedFrames.end(),
        [](const FrameData& a, const FrameData& b) {
            return a.time < b.time;
        });

    // x, y 배열 추출 (중복 제거)
    std::vector<float> x, y;
    float lastTime = -1.0f;
    for (const auto& frame : editedFrames) {
        // 같은 시간에 여러 편집이 있으면 마지막 것만 사용
        if (std::abs(frame.time - lastTime) < frameInterval_ * 0.1f) {
            // 중복: 마지막 값 업데이트
            if (!x.empty()) {
                y.back() = frame.pitchSemitones;
            }
        } else {
            // 새로운 포인트
            x.push_back(frame.time);
            y.push_back(frame.pitchSemitones);
            lastTime = frame.time;
        }
    }

    // 정렬 후 포인트가 너무 적으면 보간 불가
    if (x.size() < 2) {
        // 단일 포인트로 다시 처리
        std::vector<FrameData> result;
        float maxTime = (totalDuration_ > 0.0f) ? totalDuration_ : (x.empty() ? 0.0f : x.back() + frameInterval_);

        if (maxTime <= 0.0f) {
            return editedFrames;
        }

        float editTime = x.empty() ? -1.0f : x.front();
        float editSemitones = y.empty() ? 0.0f : y.front();

        for (float t = 0.0f; t < maxTime; t += frameInterval_) {
            FrameData frame;
            frame.time = t;
            frame.durationRatio = 1.0f;

            if (x.empty() || std::abs(t - editTime) > frameInterval_ * 2.0f) {
                frame.pitchSemitones = 0.0f;
            } else {
                frame.pitchSemitones = editSemitones;
                if (std::abs(t - editTime) < frameInterval_ / 2.0f) {
                    frame.isEdited = true;
                }
            }

            result.push_back(frame);
        }

        return result;
    }

    // Cubic spline 계수 계산
    std::vector<float> a, b, c, d;
    try {
        calculateCubicSplineCoefficients(x, y, a, b, c, d);
    } catch (const std::exception& e) {
        // 보간 실패 시 원본 반환
        return frames;
    }

    // 보간된 프레임들 생성
    std::vector<FrameData> result;

    float startTime = editedFrames.front().time;
    float endTime = editedFrames.back().time;
    float currentTime = 0.0f;

    // 첫 번째 편집 포인트 이전: pitchSemitones = 0
    while (currentTime < startTime) {
        FrameData frame;
        frame.time = currentTime;
        frame.pitchSemitones = 0.0f;
        frame.durationRatio = 1.0f;
        result.push_back(frame);
        currentTime += frameInterval_;
    }

    // 편집 범위 내: Cubic spline 보간
    while (currentTime <= endTime) {
        FrameData frame;
        frame.time = currentTime;

        // 보간 값 계산
        frame.pitchSemitones = evaluateCubicSpline(currentTime, x, a, b, c, d);
        frame.durationRatio = 1.0f;

        // 편집된 포인트인지 확인
        bool isEditPoint = false;
        for (const auto& editFrame : editedFrames) {
            if (std::abs(editFrame.time - currentTime) < frameInterval_ / 2.0f) {
                frame.isEdited = true;
                frame.isOutlier = editFrame.isOutlier;  // Outlier 정보 유지
                frame.editTime = editFrame.time;  // 원본 편집 시간 저장 (JS에서 pitchEdits 키로 사용)
                isEditPoint = true;
                break;
            }
        }

        if (!isEditPoint) {
            frame.isInterpolated = true;
        }

        result.push_back(frame);
        currentTime += frameInterval_;
    }

    // 마지막 편집 포인트 이후: pitchSemitones = 0
    // totalDuration_이 설정되어 있으면 그때까지, 아니면 마지막 편집 포인트 이후 1프레임까지
    float maxTime = (totalDuration_ > 0.0f)
        ? totalDuration_
        : (endTime + frameInterval_);  // editedFrames의 endTime 사용

    while (currentTime < maxTime) {
        FrameData frame;
        frame.time = currentTime;
        frame.pitchSemitones = 0.0f;
        frame.durationRatio = 1.0f;
        result.push_back(frame);
        currentTime += frameInterval_;
    }

    return result;
}

void SplineInterpolator::calculateCubicSplineCoefficients(
    const std::vector<float>& x,
    const std::vector<float>& y,
    std::vector<float>& a,
    std::vector<float>& b,
    std::vector<float>& c,
    std::vector<float>& d
) const {
    int n = x.size();
    if (n < 2) {
        throw std::invalid_argument("Need at least 2 points for spline interpolation");
    }

    // a_i = y_i
    a = y;

    // Calculate h_i = x_{i+1} - x_i
    std::vector<float> h(n - 1);
    for (int i = 0; i < n - 1; i++) {
        h[i] = x[i + 1] - x[i];
        if (h[i] <= 0) {
            throw std::invalid_argument("x values must be strictly increasing");
        }
    }

    // Linear interpolation for 2 points
    if (n == 2) {
        c.resize(n, 0.0f);
        b.resize(n - 1);
        d.resize(n - 1);

        b[0] = (y[1] - y[0]) / h[0];
        d[0] = 0.0f;
        return;
    }

    // Build tridiagonal system for c coefficients
    std::vector<float> alpha(n - 1);
    for (int i = 1; i < n - 1; i++) {
        alpha[i] = (3.0f / h[i]) * (y[i + 1] - y[i]) -
                   (3.0f / h[i - 1]) * (y[i] - y[i - 1]);
    }

    std::vector<float> l(n), mu(n), z(n);
    l[0] = 1.0f;
    mu[0] = 0.0f;
    z[0] = 0.0f;

    for (int i = 1; i < n - 1; i++) {
        l[i] = 2.0f * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
        mu[i] = h[i] / l[i];
        z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
    }

    l[n - 1] = 1.0f;
    z[n - 1] = 0.0f;

    c.resize(n);
    b.resize(n - 1);
    d.resize(n - 1);

    c[n - 1] = 0.0f;

    for (int j = n - 2; j >= 0; j--) {
        c[j] = z[j] - mu[j] * c[j + 1];
        b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0f * c[j]) / 3.0f;
        d[j] = (c[j + 1] - c[j]) / (3.0f * h[j]);
    }
}

float SplineInterpolator::evaluateCubicSpline(
    float x,
    const std::vector<float>& xPoints,
    const std::vector<float>& a,
    const std::vector<float>& b,
    const std::vector<float>& c,
    const std::vector<float>& d
) const {
    int n = xPoints.size();

    // Handle boundary cases
    if (x <= xPoints[0]) return a[0];
    if (x >= xPoints[n - 1]) return a[n - 1];

    // Binary search for correct interval
    int i = std::lower_bound(xPoints.begin(), xPoints.end(), x) - xPoints.begin() - 1;
    i = std::max(0, std::min(i, n - 2));

    // Evaluate cubic polynomial
    float dx = x - xPoints[i];
    return a[i] + b[i] * dx + c[i] * dx * dx + d[i] * dx * dx * dx;
}

std::vector<float> SplineInterpolator::solveTridiagonal(
    const std::vector<float>& a,
    const std::vector<float>& b,
    const std::vector<float>& c,
    const std::vector<float>& d
) const {
    int n = b.size();
    if (n == 0) return {};
    if (n == 1) return {d[0] / b[0]};

    std::vector<float> c_prime(n - 1);
    std::vector<float> d_prime(n);
    std::vector<float> x(n);

    // Forward elimination
    c_prime[0] = c[0] / b[0];
    d_prime[0] = d[0] / b[0];

    for (int i = 1; i < n - 1; i++) {
        float m = b[i] - a[i - 1] * c_prime[i - 1];
        c_prime[i] = c[i] / m;
        d_prime[i] = (d[i] - a[i - 1] * d_prime[i - 1]) / m;
    }

    // Last row
    float m = b[n - 1] - a[n - 2] * c_prime[n - 2];
    d_prime[n - 1] = (d[n - 1] - a[n - 2] * d_prime[n - 2]) / m;

    // Back substitution
    x[n - 1] = d_prime[n - 1];
    for (int i = n - 2; i >= 0; i--) {
        x[i] = d_prime[i] - c_prime[i] * x[i + 1];
    }

    return x;
}

void SplineInterpolator::setFrameInterval(float interval) {
    frameInterval_ = std::max(0.001f, interval);  // 최소 1ms
}

void SplineInterpolator::setSampleRate(int rate) {
    sampleRate_ = std::max(8000, rate);  // 최소 8kHz
}

void SplineInterpolator::setTotalDuration(float duration) {
    totalDuration_ = std::max(0.0f, duration);
}
