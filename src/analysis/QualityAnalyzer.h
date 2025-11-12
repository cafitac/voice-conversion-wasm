#ifndef QUALITY_ANALYZER_H
#define QUALITY_ANALYZER_H

#include <vector>
#include <string>

/**
 * QualityMetrics - 품질 분석 결과를 담는 구조체
 */
struct QualityMetrics {
    // Signal-to-Noise Ratio (dB)
    float snr;

    // Root Mean Square Error
    float rmsError;

    // Peak Error (최대 절대 오차)
    float peakError;

    // Total Harmonic Distortion (%)
    float thd;

    // Spectral Distortion (dB)
    float spectralDistortion;

    // Correlation coefficient (상관 계수)
    float correlation;

    // Processing time (ms)
    float processingTime;
};

/**
 * QualityAnalyzer - 오디오 품질 분석 클래스
 *
 * 원본 신호와 처리된 신호를 비교하여 품질 메트릭을 계산합니다.
 */
class QualityAnalyzer {
public:
    QualityAnalyzer();
    ~QualityAnalyzer();

    /**
     * 전체 품질 메트릭 계산
     * @param original 원본 신호
     * @param processed 처리된 신호
     * @param sampleRate 샘플레이트
     * @return QualityMetrics 구조체
     */
    QualityMetrics analyze(
        const std::vector<float>& original,
        const std::vector<float>& processed,
        int sampleRate
    );

    /**
     * SNR (Signal-to-Noise Ratio) 계산
     * @return SNR in dB
     */
    float calculateSNR(
        const std::vector<float>& original,
        const std::vector<float>& processed
    );

    /**
     * RMS Error 계산
     */
    float calculateRMSError(
        const std::vector<float>& original,
        const std::vector<float>& processed
    );

    /**
     * Peak Error 계산
     */
    float calculatePeakError(
        const std::vector<float>& original,
        const std::vector<float>& processed
    );

    /**
     * THD (Total Harmonic Distortion) 계산
     * @param signal 분석할 신호
     * @param sampleRate 샘플레이트
     * @param fundamentalFreq 기본 주파수 (Hz)
     * @return THD in percentage
     */
    float calculateTHD(
        const std::vector<float>& signal,
        int sampleRate,
        float fundamentalFreq
    );

    /**
     * Spectral Distortion 계산
     * @return Spectral distortion in dB
     */
    float calculateSpectralDistortion(
        const std::vector<float>& original,
        const std::vector<float>& processed,
        int sampleRate
    );

    /**
     * Correlation coefficient 계산
     * @return Correlation coefficient (0.0 ~ 1.0)
     */
    float calculateCorrelation(
        const std::vector<float>& original,
        const std::vector<float>& processed
    );

private:
    /**
     * RMS (Root Mean Square) 계산
     */
    float calculateRMS(const std::vector<float>& signal);

    /**
     * 두 신호의 길이를 맞춤 (짧은 쪽에 맞춤)
     */
    void alignSignals(
        const std::vector<float>& original,
        const std::vector<float>& processed,
        std::vector<float>& alignedOriginal,
        std::vector<float>& alignedProcessed
    );

    /**
     * FFT magnitude spectrum 계산
     */
    std::vector<float> calculateMagnitudeSpectrum(
        const std::vector<float>& signal,
        int sampleRate
    );
};

#endif // QUALITY_ANALYZER_H
