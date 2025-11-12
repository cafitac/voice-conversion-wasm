#include "QualityAnalyzer.h"
#include "utils/FFTWrapper.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

QualityAnalyzer::QualityAnalyzer() {
}

QualityAnalyzer::~QualityAnalyzer() {
}

QualityMetrics QualityAnalyzer::analyze(
    const std::vector<float>& original,
    const std::vector<float>& processed,
    int sampleRate
) {
    QualityMetrics metrics;

    // 신호 정렬
    std::vector<float> alignedOriginal, alignedProcessed;
    alignSignals(original, processed, alignedOriginal, alignedProcessed);

    // 각 메트릭 계산
    metrics.snr = calculateSNR(alignedOriginal, alignedProcessed);
    metrics.rmsError = calculateRMSError(alignedOriginal, alignedProcessed);
    metrics.peakError = calculatePeakError(alignedOriginal, alignedProcessed);
    metrics.spectralDistortion = calculateSpectralDistortion(alignedOriginal, alignedProcessed, sampleRate);
    metrics.correlation = calculateCorrelation(alignedOriginal, alignedProcessed);

    // THD는 계산 비용이 높으므로 기본값으로 설정
    // (필요시 기본 주파수를 찾아서 계산)
    metrics.thd = 0.0f;

    // Processing time은 외부에서 설정
    metrics.processingTime = 0.0f;

    return metrics;
}

float QualityAnalyzer::calculateSNR(
    const std::vector<float>& original,
    const std::vector<float>& processed
) {
    if (original.size() != processed.size() || original.empty()) {
        return 0.0f;
    }

    // Signal power
    float signalPower = 0.0f;
    for (float sample : original) {
        signalPower += sample * sample;
    }
    signalPower /= original.size();

    // Noise power (error)
    float noisePower = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float error = original[i] - processed[i];
        noisePower += error * error;
    }
    noisePower /= original.size();

    // SNR in dB
    if (noisePower < 1e-10f) {
        return 100.0f; // 매우 높은 SNR
    }

    float snr = 10.0f * std::log10(signalPower / noisePower);
    return snr;
}

float QualityAnalyzer::calculateRMSError(
    const std::vector<float>& original,
    const std::vector<float>& processed
) {
    if (original.size() != processed.size() || original.empty()) {
        return 0.0f;
    }

    float sumSquaredError = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float error = original[i] - processed[i];
        sumSquaredError += error * error;
    }

    return std::sqrt(sumSquaredError / original.size());
}

float QualityAnalyzer::calculatePeakError(
    const std::vector<float>& original,
    const std::vector<float>& processed
) {
    if (original.size() != processed.size() || original.empty()) {
        return 0.0f;
    }

    float maxError = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float error = std::abs(original[i] - processed[i]);
        maxError = std::max(maxError, error);
    }

    return maxError;
}

float QualityAnalyzer::calculateTHD(
    const std::vector<float>& signal,
    int sampleRate,
    float fundamentalFreq
) {
    // THD 계산은 복잡하므로 간소화된 버전
    // FFT를 사용하여 기본 주파수와 고조파의 크기를 비교

    if (signal.empty() || fundamentalFreq <= 0.0f) {
        return 0.0f;
    }

    int fftSize = 2048;
    FFTWrapper fft(fftSize);

    // 신호를 fftSize에 맞게 조정
    std::vector<float> paddedSignal(fftSize, 0.0f);
    size_t copySize = std::min(signal.size(), (size_t)fftSize);
    std::copy(signal.begin(), signal.begin() + copySize, paddedSignal.begin());

    // FFT 수행
    auto spectrum = fft.forward(paddedSignal);

    // 주파수 해상도
    float freqResolution = (float)sampleRate / fftSize;

    // 기본 주파수 bin
    int fundamentalBin = (int)(fundamentalFreq / freqResolution);
    if (fundamentalBin >= (int)spectrum.size()) {
        return 0.0f;
    }

    // 기본 주파수의 magnitude
    float fundamentalMag = std::abs(spectrum[fundamentalBin]);

    // 고조파들의 magnitude 합
    float harmonicsMag = 0.0f;
    for (int harmonic = 2; harmonic <= 10; ++harmonic) {
        int harmonicBin = fundamentalBin * harmonic;
        if (harmonicBin < (int)spectrum.size()) {
            harmonicsMag += std::abs(spectrum[harmonicBin]) * std::abs(spectrum[harmonicBin]);
        }
    }
    harmonicsMag = std::sqrt(harmonicsMag);

    // THD (%)
    if (fundamentalMag < 1e-6f) {
        return 0.0f;
    }

    float thd = (harmonicsMag / fundamentalMag) * 100.0f;
    return thd;
}

float QualityAnalyzer::calculateSpectralDistortion(
    const std::vector<float>& original,
    const std::vector<float>& processed,
    int sampleRate
) {
    if (original.empty() || processed.empty()) {
        return 0.0f;
    }

    // Magnitude spectrum 계산
    std::vector<float> originalSpectrum = calculateMagnitudeSpectrum(original, sampleRate);
    std::vector<float> processedSpectrum = calculateMagnitudeSpectrum(processed, sampleRate);

    // 길이 맞추기
    size_t minSize = std::min(originalSpectrum.size(), processedSpectrum.size());

    // Spectral distortion (평균 dB 차이)
    float sumDistortion = 0.0f;
    int count = 0;

    for (size_t i = 1; i < minSize; ++i) { // DC 제외
        float origMag = originalSpectrum[i];
        float procMag = processedSpectrum[i];

        if (origMag > 1e-6f && procMag > 1e-6f) {
            float distortion = 20.0f * std::log10(procMag / origMag);
            sumDistortion += std::abs(distortion);
            count++;
        }
    }

    if (count == 0) {
        return 0.0f;
    }

    return sumDistortion / count;
}

float QualityAnalyzer::calculateCorrelation(
    const std::vector<float>& original,
    const std::vector<float>& processed
) {
    if (original.size() != processed.size() || original.empty()) {
        return 0.0f;
    }

    size_t n = original.size();

    // 평균 계산
    float meanOriginal = std::accumulate(original.begin(), original.end(), 0.0f) / n;
    float meanProcessed = std::accumulate(processed.begin(), processed.end(), 0.0f) / n;

    // 공분산 및 표준편차 계산
    float covariance = 0.0f;
    float stdOriginal = 0.0f;
    float stdProcessed = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        float diffOriginal = original[i] - meanOriginal;
        float diffProcessed = processed[i] - meanProcessed;

        covariance += diffOriginal * diffProcessed;
        stdOriginal += diffOriginal * diffOriginal;
        stdProcessed += diffProcessed * diffProcessed;
    }

    stdOriginal = std::sqrt(stdOriginal);
    stdProcessed = std::sqrt(stdProcessed);

    // Correlation coefficient
    if (stdOriginal < 1e-6f || stdProcessed < 1e-6f) {
        return 0.0f;
    }

    float correlation = covariance / (stdOriginal * stdProcessed);
    return std::max(0.0f, std::min(1.0f, correlation)); // 0.0 ~ 1.0으로 클램프
}

float QualityAnalyzer::calculateRMS(const std::vector<float>& signal) {
    if (signal.empty()) {
        return 0.0f;
    }

    float sumSquares = 0.0f;
    for (float sample : signal) {
        sumSquares += sample * sample;
    }

    return std::sqrt(sumSquares / signal.size());
}

void QualityAnalyzer::alignSignals(
    const std::vector<float>& original,
    const std::vector<float>& processed,
    std::vector<float>& alignedOriginal,
    std::vector<float>& alignedProcessed
) {
    // 길이가 다를 경우 짧은 쪽에 맞춤
    size_t minSize = std::min(original.size(), processed.size());

    alignedOriginal.assign(original.begin(), original.begin() + minSize);
    alignedProcessed.assign(processed.begin(), processed.begin() + minSize);
}

std::vector<float> QualityAnalyzer::calculateMagnitudeSpectrum(
    const std::vector<float>& signal,
    int sampleRate
) {
    int fftSize = 2048;
    FFTWrapper fft(fftSize);

    // 신호를 fftSize에 맞게 조정
    std::vector<float> paddedSignal(fftSize, 0.0f);
    size_t copySize = std::min(signal.size(), (size_t)fftSize);
    std::copy(signal.begin(), signal.begin() + copySize, paddedSignal.begin());

    // FFT 수행
    auto spectrum = fft.forward(paddedSignal);

    // Magnitude 계산
    std::vector<float> magnitudes;
    magnitudes.reserve(spectrum.size());

    for (const auto& complexValue : spectrum) {
        float magnitude = std::abs(complexValue);
        magnitudes.push_back(magnitude);
    }

    return magnitudes;
}
