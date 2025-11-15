#ifndef PHASEVOCODERPITCHPROCESSOR_H
#define PHASEVOCODERPITCHPROCESSOR_H

#include "IPitchProcessor.h"
#include "../../effects/PhaseVocoderPitchShifter.h"
#include <memory>

/**
 * PhaseVocoderPitchProcessor
 *
 * Phase Vocoder 기반 Variable Pitch 프로세서
 *
 * 특징:
 * - 네이티브 variable pitch 지원 (각 STFT 프레임마다 다른 shift)
 * - 최고 품질 (큰 변조량에도 자연스러움)
 * - Formant preservation 지원
 * - 음악/음성 모두 우수
 *
 * 동작 원리:
 * 1. STFT (Short-Time Fourier Transform)
 * 2. 각 STFT 프레임의 시간에서 FrameData의 pitchSemitones 읽기
 * 3. Frequency bin shifting with phase coherence
 * 4. ISTFT (Inverse STFT)
 *
 * Variable pitch의 핵심:
 *   기존: 모든 STFT 프레임에 동일한 pitch ratio 적용
 *   Variable: 각 STFT 프레임마다 해당 시간의 pitchSemitones 사용
 *
 * 장점:
 * - 최고 품질
 * - 큰 변조량 (±12 semitones)도 자연스러움
 * - Formant preservation으로 음색 유지
 *
 * 단점:
 * - 높은 연산량 (FFT/IFFT)
 * - 메모리 사용량 많음
 * - 레이턴시 있음 (~43ms at 48kHz)
 */
class PhaseVocoderPitchProcessor : public IPitchProcessor {
public:
    /**
     * @param fftSize FFT 크기 (기본 2048)
     * @param hopSize Hop 크기 (기본 512, 75% overlap)
     * @param formantPreservation Formant preservation 활성화 (기본 true)
     */
    PhaseVocoderPitchProcessor(
        int fftSize = 2048,
        int hopSize = 512,
        bool formantPreservation = true
    );

    ~PhaseVocoderPitchProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    bool supportsVariablePitch() const override { return true; }

    const char* getName() const override {
        return "Phase Vocoder Pitch Processor";
    }

    const char* getDescription() const override {
        return "Highest quality, native variable pitch, large pitch shifts";
    }

    /**
     * Formant preservation 설정
     */
    void setFormantPreservation(bool enabled);

private:
    std::unique_ptr<PhaseVocoderPitchShifter> shifter_;
    int fftSize_;
    int hopSize_;
    bool formantPreservation_;

    /**
     * Variable pitch shift 적용
     *
     * 각 STFT 프레임마다 다른 pitch ratio 적용
     *
     * @param audio 입력 오디오
     * @param frames FrameData (pitchSemitones 정보)
     * @param sampleRate 샘플 레이트
     * @return 처리된 오디오
     */
    std::vector<float> processVariablePitch(
        const std::vector<float>& audio,
        const std::vector<FrameData>& frames,
        int sampleRate
    );

    /**
     * 특정 시간의 pitchSemitones 가져오기
     */
    float getPitchSemitonesAtTime(
        float time,
        const std::vector<FrameData>& frames
    ) const;

    /**
     * FrameData를 연속 오디오로 변환
     */
    std::vector<float> framesToAudio(const std::vector<FrameData>& frames) const;

    /**
     * 오디오를 FrameData로 변환
     */
    std::vector<FrameData> audioToFrames(
        const std::vector<float>& audio,
        const std::vector<FrameData>& originalFrames,
        int sampleRate
    ) const;
};

#endif // PHASEVOCODERPITCHPROCESSOR_H
