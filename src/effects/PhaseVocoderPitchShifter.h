#ifndef PHASEVOCODERPITCHSHIFTER_H
#define PHASEVOCODERPITCHSHIFTER_H

#include "../audio/AudioBuffer.h"
#include "PhaseVocoder.h"
#include <vector>
#include <complex>
#include <memory>

/**
 * PhaseVocoderPitchShifter
 *
 * Phase Vocoder 알고리즘을 사용한 고품질 pitch shifting
 * Wikipedia "Audio time stretching and pitch scaling" 참조
 *
 * 주요 개선 사항:
 * - Phase coherence 유지로 phasey/smeared 소리 제거
 * - Formant preservation으로 metallic/robotic 소리 제거
 * - Anti-aliasing으로 high-frequency 노이즈 제거
 */
class PhaseVocoderPitchShifter {
public:
    /**
     * @param fftSize FFT 크기 (기본 2048)
     * @param hopSize Hop 크기 (기본 512, 75% overlap)
     */
    PhaseVocoderPitchShifter(int fftSize = 2048, int hopSize = 512);
    ~PhaseVocoderPitchShifter();

    /**
     * Pitch shift 적용 (Phase Vocoder 알고리즘 사용)
     *
     * @param input 입력 오디오 버퍼
     * @param semitones Pitch shift 양 (semitones 단위)
     * @return Pitch shifted 오디오 버퍼
     */
    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones);

    /**
     * Formant preservation 활성화/비활성화
     */
    void setFormantPreservation(bool enabled) { formantPreservation_ = enabled; }

    /**
     * Anti-aliasing filter 활성화/비활성화
     */
    void setAntiAliasing(bool enabled) { antiAliasing_ = enabled; }

private:
    std::unique_ptr<PhaseVocoder> vocoder_;
    bool formantPreservation_;
    bool antiAliasing_;

    // Phase vocoder state
    std::vector<float> previousPhase_;

    /**
     * Pitch shifting with phase coherence
     */
    std::vector<std::vector<std::complex<float>>> shiftSpectrum(
        const std::vector<std::vector<std::complex<float>>>& stft,
        float pitchRatio
    );

    /**
     * Spectral envelope 추출 (formant preservation용)
     */
    std::vector<float> extractSpectralEnvelope(
        const std::vector<std::complex<float>>& spectrum
    );

    /**
     * Apply spectral envelope (formant preservation용)
     */
    void applySpectralEnvelope(
        std::vector<std::complex<float>>& spectrum,
        const std::vector<float>& envelope
    );

    /**
     * Low-pass filter (anti-aliasing용)
     */
    std::vector<float> lowPassFilter(
        const std::vector<float>& signal,
        float cutoffFreq,
        int sampleRate
    );

    /**
     * Phase unwrapping (phase coherence용)
     */
    float unwrapPhase(float phase, float previousPhase);
};

#endif // PHASEVOCODERPITCHSHIFTER_H
