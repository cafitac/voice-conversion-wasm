#include "PitchCurveInterpolator.h"
#include <algorithm>
#include <stdexcept>

/**
 * Tridiagonal matrix solver (Thomas algorithm)
 *
 * Solves Ax = d where A is a tridiagonal matrix
 * Time complexity: O(n)
 *
 * @param a Lower diagonal (size n-1)
 * @param b Main diagonal (size n)
 * @param c Upper diagonal (size n-1)
 * @param d Right-hand side (size n)
 * @return Solution vector x (size n)
 */
std::vector<float> PitchCurveInterpolator::solveTridiagonal(
    const std::vector<float>& a,
    const std::vector<float>& b,
    const std::vector<float>& c,
    const std::vector<float>& d
) {
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

/**
 * Calculate cubic spline coefficients
 *
 * Uses natural cubic spline (2nd derivative = 0 at endpoints)
 * Ensures C2 continuity (continuous up to 2nd derivative)
 *
 * For each interval [x_i, x_{i+1}], the spline is:
 * S_i(x) = a_i + b_i*(x-x_i) + c_i*(x-x_i)^2 + d_i*(x-x_i)^3
 */
void PitchCurveInterpolator::calculateCubicSplineCoefficients(
    const std::vector<float>& x,
    const std::vector<float>& y,
    std::vector<float>& a,
    std::vector<float>& b,
    std::vector<float>& c,
    std::vector<float>& d
) {
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

    // Solve for c coefficients using tridiagonal system
    // Natural spline boundary conditions: c[0] = c[n-1] = 0
    if (n == 2) {
        // Linear interpolation for 2 points
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

/**
 * Evaluate cubic spline at given x
 *
 * Uses binary search to find the correct interval
 * Then evaluates: f(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
 */
float PitchCurveInterpolator::evaluateCubicSpline(
    float x,
    const std::vector<float>& xPoints,
    const std::vector<float>& a,
    const std::vector<float>& b,
    const std::vector<float>& c,
    const std::vector<float>& d
) {
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

/**
 * Get semitones at specific time using cubic spline interpolation
 *
 * @param time Time in seconds
 * @param editPoints Array of edit points (must be sorted by time)
 * @return Interpolated semitones value
 *
 * Returns 0 if:
 * - No edit points
 * - Time is before first edit point
 * - Time is after last edit point
 */
float PitchCurveInterpolator::getSemitonesAtTime(
    float time,
    const std::vector<PitchEditPoint>& editPoints
) {
    if (editPoints.empty()) return 0.0f;
    if (editPoints.size() == 1) {
        // Single point: return value if exactly at that time, else 0
        return (std::abs(time - editPoints[0].time) < 0.001f) ? editPoints[0].semitones : 0.0f;
    }

    // Check if time is outside edited range
    if (time < editPoints.front().time || time > editPoints.back().time) {
        return 0.0f;
    }

    // Extract x and y arrays
    std::vector<float> x, y;
    for (const auto& point : editPoints) {
        x.push_back(point.time);
        y.push_back(point.semitones);
    }

    // Calculate spline coefficients
    std::vector<float> a, b, c, d;
    calculateCubicSplineCoefficients(x, y, a, b, c, d);

    // Evaluate at requested time
    return evaluateCubicSpline(time, x, a, b, c, d);
}

/**
 * Generate pitch curve for entire audio using cubic spline interpolation
 *
 * @param editPoints Array of edit points (must be sorted by time)
 * @param totalSamples Total number of samples in audio
 * @param sampleRate Sample rate in Hz
 * @return Vector of semitones values (one per sample)
 *
 * Algorithm:
 * 1. If no edit points, return all zeros (no pitch change)
 * 2. Build cubic spline through edit points
 * 3. For each sample:
 *    - Convert sample index to time
 *    - If outside edited range: semitones = 0
 *    - If inside edited range: evaluate spline
 *
 * This allows pitch shift algorithms to apply variable pitch shift
 * by reading semitones[sampleIndex] for each sample.
 */
std::vector<float> PitchCurveInterpolator::interpolatePitchCurve(
    const std::vector<PitchEditPoint>& editPoints,
    int totalSamples,
    int sampleRate
) {
    std::vector<float> curve(totalSamples, 0.0f);

    if (editPoints.empty() || totalSamples <= 0 || sampleRate <= 0) {
        return curve;
    }

    if (editPoints.size() == 1) {
        // Single point: apply constant semitones value at that exact time
        int sampleIndex = static_cast<int>(editPoints[0].time * sampleRate);
        if (sampleIndex >= 0 && sampleIndex < totalSamples) {
            curve[sampleIndex] = editPoints[0].semitones;
        }
        return curve;
    }

    // Extract x and y arrays
    std::vector<float> x, y;
    for (const auto& point : editPoints) {
        x.push_back(point.time);
        y.push_back(point.semitones);
    }

    // Calculate spline coefficients once
    std::vector<float> a, b, c, d;
    try {
        calculateCubicSplineCoefficients(x, y, a, b, c, d);
    } catch (const std::exception& e) {
        // If spline calculation fails, return zero curve
        return curve;
    }

    // Find sample range that corresponds to edited region
    float startTime = editPoints.front().time;
    float endTime = editPoints.back().time;
    int startSample = static_cast<int>(startTime * sampleRate);
    int endSample = static_cast<int>(endTime * sampleRate);

    // Clamp to valid range
    startSample = std::max(0, startSample);
    endSample = std::min(totalSamples - 1, endSample);

    // Evaluate spline for each sample in edited range
    for (int i = startSample; i <= endSample; i++) {
        float time = static_cast<float>(i) / sampleRate;
        curve[i] = evaluateCubicSpline(time, x, a, b, c, d);
    }

    // Samples outside edited range remain 0 (initialized above)
    return curve;
}
