#include "PhaseVocoderPitchShifter.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PhaseVocoderPitchShifter::PhaseVocoderPitchShifter(int fftSize, int hopSize)
    : vocoder_(new PhaseVocoder(fftSize, hopSize)),
      formantPreservation_(false), antiAliasing_(true),  // Formant preservation OFF for now
      previousPhase_(fftSize / 2 + 1, 0.0f) {
}

PhaseVocoderPitchShifter::~PhaseVocoderPitchShifter() {
}

float PhaseVocoderPitchShifter::unwrapPhase(float phase, float previousPhase) {
    // Phase unwrapping: 위상 불연속성 제거
    float delta = phase - previousPhase;

    // Wrap delta to [-π, π] range
    while (delta > M_PI) delta -= 2.0f * M_PI;
    while (delta < -M_PI) delta += 2.0f * M_PI;

    return previousPhase + delta;
}

std::vector<std::vector<std::complex<float>>> PhaseVocoderPitchShifter::shiftSpectrum(
    const std::vector<std::vector<std::complex<float>>>& stft,
    float pitchRatio
) {
    if (stft.empty()) {
        return stft;
    }

    int numFrames = stft.size();
    int numBins = stft[0].size();

    std::vector<std::vector<std::complex<float>>> shiftedSTFT(numFrames);

    // Formant preservation: spectral envelope 추출 (옵션)
    std::vector<std::vector<float>> envelopes;
    if (formantPreservation_) {
        for (const auto& frame : stft) {
            envelopes.push_back(extractSpectralEnvelope(frame));
        }
    }

    // 각 프레임에 대해 pitch shifting 적용
    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
        std::vector<std::complex<float>> shiftedFrame(numBins, std::complex<float>(0.0f, 0.0f));

        // Frequency bin resampling with phase coherence
        for (int bin = 0; bin < numBins; ++bin) {
            // 소스 bin 계산 (pitch를 올리려면 낮은 bin의 값을 높은 bin으로)
            float sourceBin = bin * pitchRatio;

            if (sourceBin >= 0 && sourceBin < numBins - 1) {
                // Linear interpolation for magnitude & phase
                int bin0 = static_cast<int>(sourceBin);
                int bin1 = bin0 + 1;
                float frac = sourceBin - bin0;

                std::complex<float> val0 = stft[frameIdx][bin0];
                std::complex<float> val1 = stft[frameIdx][bin1];

                // Magnitude interpolation
                float mag0 = std::abs(val0);
                float mag1 = std::abs(val1);
                float mag = mag0 * (1.0f - frac) + mag1 * frac;

                // Phase interpolation with coherence
                float phase0 = std::arg(val0);
                float phase1 = std::arg(val1);

                // Phase unwrapping for coherence
                phase1 = unwrapPhase(phase1, phase0);
                float phase = phase0 * (1.0f - frac) + phase1 * frac;

                // Reconstruct complex value
                shiftedFrame[bin] = std::polar(mag, phase);
            }
        }

        // Formant preservation: shifted된 스펙트럼의 envelope를 원래 envelope로 변경
        // 주의: envelope도 pitch ratio에 맞춰 shift되어야 합니다
        if (formantPreservation_ && frameIdx < static_cast<int>(envelopes.size())) {
            // Envelope도 pitch shift 적용
            std::vector<float> shiftedEnvelope(numBins, 0.0f);
            for (int bin = 0; bin < numBins; ++bin) {
                float sourceBin = bin * pitchRatio;  // envelope는 반대 방향
                if (sourceBin >= 0 && sourceBin < envelopes[frameIdx].size() - 1) {
                    int bin0 = static_cast<int>(sourceBin);
                    int bin1 = bin0 + 1;
                    float frac = sourceBin - bin0;
                    shiftedEnvelope[bin] = envelopes[frameIdx][bin0] * (1.0f - frac) +
                                          envelopes[frameIdx][bin1] * frac;
                }
            }
            applySpectralEnvelope(shiftedFrame, shiftedEnvelope);
        }

        shiftedSTFT[frameIdx] = shiftedFrame;
    }

    return shiftedSTFT;
}

std::vector<float> PhaseVocoderPitchShifter::extractSpectralEnvelope(
    const std::vector<std::complex<float>>& spectrum
) {
    // Simplified spectral envelope extraction
    // 실제로는 LPC나 cepstral analysis를 사용하지만, 여기서는 간단한 smoothing 사용
    std::vector<float> envelope(spectrum.size());

    // Magnitude 추출
    for (size_t i = 0; i < spectrum.size(); ++i) {
        envelope[i] = std::abs(spectrum[i]);
    }

    // Moving average smoothing
    int windowSize = 11; // Smoothing window
    std::vector<float> smoothed(envelope.size());
    for (size_t i = 0; i < envelope.size(); ++i) {
        float sum = 0.0f;
        int count = 0;
        for (int j = -windowSize / 2; j <= windowSize / 2; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < static_cast<int>(envelope.size())) {
                sum += envelope[idx];
                count++;
            }
        }
        smoothed[i] = sum / count;
    }

    return smoothed;
}

void PhaseVocoderPitchShifter::applySpectralEnvelope(
    std::vector<std::complex<float>>& spectrum,
    const std::vector<float>& envelope
) {
    for (size_t i = 0; i < spectrum.size() && i < envelope.size(); ++i) {
        float currentMag = std::abs(spectrum[i]);
        if (currentMag > 0.0f && envelope[i] > 0.0f) {
            // Preserve phase, adjust magnitude to match envelope
            float phase = std::arg(spectrum[i]);
            spectrum[i] = std::polar(envelope[i], phase);
        }
    }
}

std::vector<float> PhaseVocoderPitchShifter::lowPassFilter(
    const std::vector<float>& signal,
    float cutoffFreq,
    int sampleRate
) {
    // Simple single-pole IIR low-pass filter
    float rc = 1.0f / (2.0f * M_PI * cutoffFreq);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (rc + dt);

    std::vector<float> filtered(signal.size());
    filtered[0] = signal[0];

    for (size_t i = 1; i < signal.size(); ++i) {
        filtered[i] = filtered[i - 1] + alpha * (signal[i] - filtered[i - 1]);
    }

    return filtered;
}

AudioBuffer PhaseVocoderPitchShifter::shiftPitch(const AudioBuffer& input, float semitones) {
    // Pitch ratio 계산
    float pitchRatio = std::pow(2.0f, semitones / 12.0f);

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();

    // Anti-aliasing pre-filter (pitch를 올릴 때)
    std::vector<float> processedInput = inputData;
    if (antiAliasing_ && pitchRatio > 1.0f) {
        float cutoff = sampleRate / (2.0f * pitchRatio);
        processedInput = lowPassFilter(inputData, cutoff, sampleRate);
    }

    // STFT 분석 (PhaseVocoder 사용)
    auto stft = vocoder_->analyzeSTFT(processedInput);

    // Pitch shifting with phase coherence
    auto shiftedSTFT = shiftSpectrum(stft, pitchRatio);

    // ISTFT 합성 (PhaseVocoder 사용)
    auto outputData = vocoder_->synthesizeISTFT(shiftedSTFT, inputData.size());

    // Anti-aliasing post-filter (pitch를 내릴 때)
    if (antiAliasing_ && pitchRatio < 1.0f) {
        float cutoff = sampleRate / 2.0f * pitchRatio;
        outputData = lowPassFilter(outputData, cutoff, sampleRate);
    }

    // AudioBuffer 생성
    AudioBuffer output(sampleRate, input.getChannels());
    output.setData(outputData);

    return output;
}
