#include "PhaseVocoder.h"
#include "../utils/FFTWrapper.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PhaseVocoder::PhaseVocoder(int fftSize, int hopSize)
    : fftSize_(fftSize), hopSize_(hopSize) {
}

PhaseVocoder::~PhaseVocoder() {
}

std::vector<float> PhaseVocoder::createHanningWindow(int size) {
    std::vector<float> window(size);
    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }
    return window;
}

std::vector<std::vector<std::complex<float>>> PhaseVocoder::analyzeSTFT(
    const std::vector<float>& signal
) {
    FFTWrapper fft(fftSize_);
    std::vector<float> window = createHanningWindow(fftSize_);

    std::vector<std::vector<std::complex<float>>> stft;

    // Hop size만큼 이동하면서 FFT 수행
    for (size_t i = 0; i + fftSize_ <= signal.size(); i += hopSize_) {
        // 프레임 추출 및 윈도우 적용
        std::vector<float> frame(fftSize_);
        for (int j = 0; j < fftSize_; ++j) {
            frame[j] = signal[i + j] * window[j];
        }

        // FFT
        auto spectrum = fft.forward(frame);
        stft.push_back(spectrum);
    }

    return stft;
}

std::vector<float> PhaseVocoder::synthesizeISTFT(
    const std::vector<std::vector<std::complex<float>>>& stft,
    int outputLength
) {
    if (stft.empty()) {
        return std::vector<float>(outputLength, 0.0f);
    }

    FFTWrapper fft(fftSize_);
    std::vector<float> window = createHanningWindow(fftSize_);

    // 출력 버퍼 초기화
    std::vector<float> output(outputLength, 0.0f);
    std::vector<float> windowSum(outputLength, 0.0f);

    // Overlap-Add 합성
    for (size_t frameIdx = 0; frameIdx < stft.size(); ++frameIdx) {
        size_t position = frameIdx * hopSize_;

        // IFFT
        auto timeDomain = fft.inverse(stft[frameIdx]);

        // 윈도우 적용 및 overlap-add
        for (int i = 0; i < fftSize_ && position + i < static_cast<size_t>(outputLength); ++i) {
            output[position + i] += timeDomain[i] * window[i];
            windowSum[position + i] += window[i];
        }
    }

    // 정규화 (window sum으로 나누기)
    for (size_t i = 0; i < output.size(); ++i) {
        if (windowSum[i] > 0.0f) {
            output[i] /= windowSum[i];
        }
    }

    return output;
}
