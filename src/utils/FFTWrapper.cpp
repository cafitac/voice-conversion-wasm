#include "FFTWrapper.h"
#include "../external/kissfft/kiss_fft.h"
#include <cstring>

FFTWrapper::FFTWrapper(int size) : fftSize_(size) {
    // Forward FFT configuration
    fftCfg_ = kiss_fft_alloc(fftSize_, 0, nullptr, nullptr);

    // Inverse FFT configuration
    ifftCfg_ = kiss_fft_alloc(fftSize_, 1, nullptr, nullptr);
}

FFTWrapper::~FFTWrapper() {
    if (fftCfg_) {
        kiss_fft_free(fftCfg_);
    }
    if (ifftCfg_) {
        kiss_fft_free(ifftCfg_);
    }
}

std::vector<std::complex<float>> FFTWrapper::forward(const std::vector<float>& input) {
    // 입력 크기 확인
    if (input.size() != static_cast<size_t>(fftSize_)) {
        // 크기가 다르면 zero-padding 또는 truncate
        std::vector<float> paddedInput(fftSize_, 0.0f);
        size_t copySize = std::min(input.size(), static_cast<size_t>(fftSize_));
        std::copy(input.begin(), input.begin() + copySize, paddedInput.begin());
        return forward(paddedInput);
    }

    // KissFFT의 복소수 포맷으로 변환 (실수 입력 → 복소수)
    std::vector<kiss_fft_cpx> inputCpx(fftSize_);
    for (int i = 0; i < fftSize_; ++i) {
        inputCpx[i].r = input[i];
        inputCpx[i].i = 0.0f;
    }

    // FFT 수행
    std::vector<kiss_fft_cpx> outputCpx(fftSize_);
    kiss_fft(static_cast<kiss_fft_cfg>(fftCfg_), inputCpx.data(), outputCpx.data());

    // std::complex<float> 포맷으로 변환
    std::vector<std::complex<float>> output(fftSize_);
    for (int i = 0; i < fftSize_; ++i) {
        output[i] = std::complex<float>(outputCpx[i].r, outputCpx[i].i);
    }

    return output;
}

std::vector<float> FFTWrapper::inverse(const std::vector<std::complex<float>>& input) {
    // 입력 크기 확인
    if (input.size() != static_cast<size_t>(fftSize_)) {
        std::vector<std::complex<float>> paddedInput(fftSize_, std::complex<float>(0.0f, 0.0f));
        size_t copySize = std::min(input.size(), static_cast<size_t>(fftSize_));
        std::copy(input.begin(), input.begin() + copySize, paddedInput.begin());
        return inverse(paddedInput);
    }

    // std::complex<float> → KissFFT 복소수 포맷으로 변환
    std::vector<kiss_fft_cpx> inputCpx(fftSize_);
    for (int i = 0; i < fftSize_; ++i) {
        inputCpx[i].r = input[i].real();
        inputCpx[i].i = input[i].imag();
    }

    // IFFT 수행
    std::vector<kiss_fft_cpx> outputCpx(fftSize_);
    kiss_fft(static_cast<kiss_fft_cfg>(ifftCfg_), inputCpx.data(), outputCpx.data());

    // 실수 부분만 추출 + normalization (KissFFT는 자동 정규화 안함)
    std::vector<float> output(fftSize_);
    float scale = 1.0f / fftSize_;
    for (int i = 0; i < fftSize_; ++i) {
        output[i] = outputCpx[i].r * scale;
    }

    return output;
}
