#ifndef PSOLAALGORITHM_H
#define PSOLAALGORITHM_H

#include "IPitchAlgorithm.h"
#include <vector>

/**
 * PSOLAAlgorithm
 *
 * Pitch Synchronous Overlap-Add 알고리즘
 *
 * 특징:
 * - Time-domain 처리
 * - 빠른 처리 속도 (1-2초)
 * - 음성에 최적화
 * - Native variable pitch 지원
 *
 * 동작 원리:
 * 1. Pitch mark 검출 (autocorrelation)
 * 2. 각 pitch period를 grain으로 추출
 * 3. Grain을 재배치하여 pitch 변경
 * 4. Overlap-add로 합성
 *
 * 장점:
 * - 빠름
 * - 음성에서 자연스러운 결과
 * - Real-time 처리 가능
 *
 * 단점:
 * - 음악/복잡한 소리에는 부적합
 * - Pitch mark 검출 실패 시 artifacts
 */
class PSOLAAlgorithm : public IPitchAlgorithm {
public:
    /**
     * @param windowSize Pitch 검출 윈도우 크기 (기본 2048)
     * @param hopSize Hop 크기 (기본 512)
     */
    PSOLAAlgorithm(int windowSize = 2048, int hopSize = 512);
    ~PSOLAAlgorithm() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;

    const char* getName() const override {
        return "PSOLA (Pitch Synchronous Overlap-Add)";
    }

    const char* getDescription() const override {
        return "Fast, time-domain, optimized for voice";
    }

    bool supportsRealtime() const override { return true; }

private:
    int windowSize_;
    int hopSize_;

    /**
     * Pitch mark 검출 (autocorrelation 기반)
     */
    std::vector<int> detectPitchMarks(
        const std::vector<float>& audio,
        int sampleRate
    ) const;

    /**
     * Autocorrelation 계산
     */
    std::vector<float> computeAutocorrelation(
        const std::vector<float>& signal,
        int maxLag
    ) const;

    /**
     * PSOLA pitch shift (fixed pitch)
     */
    std::vector<float> psolaShift(
        const std::vector<float>& audio,
        const std::vector<int>& pitchMarks,
        float pitchScale,
        int sampleRate
    ) const;

    /**
     * Hanning window 생성
     */
    std::vector<float> hanningWindow(int size) const;
};

#endif // PSOLAALGORITHM_H
