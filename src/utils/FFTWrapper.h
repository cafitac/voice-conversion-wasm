#ifndef FFTWRAPPER_H
#define FFTWRAPPER_H

#include <vector>
#include <complex>

/**
 * FFTWrapper
 *
 * KissFFT 라이브러리를 래핑하여 사용하기 쉽게 만든 유틸리티 클래스
 * Phase Vocoder 구현을 위한 FFT/IFFT 기능 제공
 */
class FFTWrapper {
public:
    FFTWrapper(int size);
    ~FFTWrapper();

    /**
     * Forward FFT (시간 영역 → 주파수 영역)
     *
     * @param input 시간 영역 실수 샘플
     * @return 주파수 영역 복소수 (magnitude + phase)
     */
    std::vector<std::complex<float>> forward(const std::vector<float>& input);

    /**
     * Inverse FFT (주파수 영역 → 시간 영역)
     *
     * @param input 주파수 영역 복소수
     * @return 시간 영역 실수 샘플
     */
    std::vector<float> inverse(const std::vector<std::complex<float>>& input);

    /**
     * FFT 크기 반환
     */
    int getSize() const { return fftSize_; }

private:
    int fftSize_;
    void* fftCfg_;      // kiss_fft_cfg (forward)
    void* ifftCfg_;     // kiss_fft_cfg (inverse)
};

#endif // FFTWRAPPER_H
