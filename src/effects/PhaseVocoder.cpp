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

std::vector<float> PhaseVocoder::timeStretch(
    const std::vector<float>& signal,
    float ratio
) {
    if (signal.empty() || ratio <= 0.0f) {
        return signal;
    }

    // Analysis (고정 hop size)
    int analysisHop = hopSize_;

    // Synthesis (ratio에 따라 조정)
    int synthesisHop = static_cast<int>(analysisHop * ratio);

    // STFT 분석
    auto stft = analyzeSTFT(signal);

    if (stft.empty()) {
        return signal;
    }

    int numBins = stft[0].size();

    // Phase tracking 초기화
    std::vector<float> previousInputPhase(numBins, 0.0f);
    std::vector<float> previousOutputPhase(numBins, 0.0f);

    // Phase 조정된 STFT
    std::vector<std::vector<std::complex<float>>> stretchedSTFT;

    for (size_t frameIdx = 0; frameIdx < stft.size(); ++frameIdx) {
        auto& spectrum = stft[frameIdx];
        std::vector<float> currentInputPhase(numBins);
        std::vector<float> currentOutputPhase(numBins);

        // Phase propagation 수행
        for (int bin = 0; bin < numBins; ++bin) {
            float magnitude = std::abs(spectrum[bin]);
            float phase = std::arg(spectrum[bin]);

            // 현재 입력 phase 저장
            currentInputPhase[bin] = phase;

            if (frameIdx == 0) {
                // 첫 프레임: phase 그대로 사용
                currentOutputPhase[bin] = phase;
            } else {
                // Phase unwrapping and instantaneous frequency 계산
                float expectedPhase = previousInputPhase[bin] +
                                     (analysisHop * 2.0f * M_PI * bin / fftSize_);

                // Phase difference (wrap to [-π, π])
                float phaseDiff = phase - expectedPhase;
                while (phaseDiff > M_PI) phaseDiff -= 2.0f * M_PI;
                while (phaseDiff < -M_PI) phaseDiff += 2.0f * M_PI;

                // True frequency (instantaneous)
                float trueFreq = (2.0f * M_PI * bin / fftSize_) + (phaseDiff / analysisHop);

                // Output phase (synthesis hop에 맞게 조정)
                currentOutputPhase[bin] = previousOutputPhase[bin] + synthesisHop * trueFreq;
            }

            // 새로운 complex number 생성 (magnitude 유지, phase 조정)
            spectrum[bin] = std::polar(magnitude, currentOutputPhase[bin]);
        }

        stretchedSTFT.push_back(spectrum);
        previousInputPhase = currentInputPhase;
        previousOutputPhase = currentOutputPhase;
    }

    // ISTFT 합성 (synthesis hop 사용)
    FFTWrapper fft(fftSize_);
    std::vector<float> window = createHanningWindow(fftSize_);

    // 출력 길이 계산
    int outputLength = static_cast<int>(signal.size() * ratio);
    std::vector<float> output(outputLength, 0.0f);
    std::vector<float> windowSum(outputLength, 0.0f);

    // Overlap-Add 합성 (synthesis hop 사용)
    for (size_t frameIdx = 0; frameIdx < stretchedSTFT.size(); ++frameIdx) {
        size_t position = frameIdx * synthesisHop;

        // IFFT
        auto timeDomain = fft.inverse(stretchedSTFT[frameIdx]);

        // 윈도우 적용 및 overlap-add
        for (int i = 0; i < fftSize_ && position + i < static_cast<size_t>(outputLength); ++i) {
            output[position + i] += timeDomain[i] * window[i];
            windowSum[position + i] += window[i];
        }
    }

    // 정규화
    for (size_t i = 0; i < output.size(); ++i) {
        if (windowSum[i] > 0.0f) {
            output[i] /= windowSum[i];
        }
    }

    return output;
}

void PhaseVocoder::propagatePhase(
    std::vector<std::complex<float>>& spectrum,
    const std::vector<float>& previousPhase,
    std::vector<float>& currentPhase,
    float phaseAdvance
) {
    // Helper function for phase propagation
    // (현재는 timeStretch에서 직접 구현)
}
