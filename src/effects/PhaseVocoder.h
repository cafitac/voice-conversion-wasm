#ifndef PHASEVOCODER_H
#define PHASEVOCODER_H

#include <vector>
#include <complex>

/**
 * PhaseVocoder
 *
 * STFT/ISTFT를 사용한 Phase Vocoder 핵심 기능
 * Wikipedia "Audio time stretching and pitch scaling" 참조
 *
 * 다른 효과들(pitch shifting, time stretching 등)에서 재사용 가능
 */
class PhaseVocoder {
public:
    /**
     * @param fftSize FFT 크기 (기본 2048)
     * @param hopSize Hop 크기 (기본 512, 75% overlap)
     */
    PhaseVocoder(int fftSize = 2048, int hopSize = 512);
    ~PhaseVocoder();

    /**
     * STFT 분석 (Short-Time Fourier Transform)
     *
     * @param signal 시간 영역 신호
     * @return 시간-주파수 영역 스펙트럼 (프레임 x 주파수 bins)
     */
    std::vector<std::vector<std::complex<float>>> analyzeSTFT(
        const std::vector<float>& signal
    );

    /**
     * ISTFT 합성 (Inverse STFT with Overlap-Add)
     *
     * @param stft 시간-주파수 영역 스펙트럼
     * @param outputLength 출력 길이 (샘플 수)
     * @return 시간 영역 신호
     */
    std::vector<float> synthesizeISTFT(
        const std::vector<std::vector<std::complex<float>>>& stft,
        int outputLength
    );

    /**
     * FFT/Hop 크기 반환
     */
    int getFFTSize() const { return fftSize_; }
    int getHopSize() const { return hopSize_; }

private:
    int fftSize_;
    int hopSize_;

    /**
     * Hanning window 생성
     */
    std::vector<float> createHanningWindow(int size);
};

#endif // PHASEVOCODER_H
