#ifndef WSOLAALGORITHM_H
#define WSOLAALGORITHM_H

#include "IDurationAlgorithm.h"

/**
 * WSOLAAlgorithm
 *
 * Waveform Similarity Overlap-Add 알고리즘
 *
 * 특징:
 * - Time-domain time stretching
 * - 빠른 처리 속도
 * - 음성에 적합
 *
 * 동작 원리:
 * 1. 입력을 grain으로 분할
 * 2. 유사도가 높은 지점에서 overlap
 * 3. Crossfade로 자연스럽게 연결
 *
 * 장점:
 * - 빠름
 * - 음성에서 자연스러움
 * - Real-time 처리 가능
 *
 * 단점:
 * - 음악에는 부적합
 * - Pitch 변화 발생 가능 (극단적 ratio에서)
 */
class WSOLAAlgorithm : public IDurationAlgorithm {
public:
    /**
     * @param windowSize 윈도우 크기 (기본 1024)
     * @param hopSize Hop 크기 (기본 512)
     */
    WSOLAAlgorithm(int windowSize = 1024, int hopSize = 512);
    ~WSOLAAlgorithm() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;

    const char* getName() const override {
        return "WSOLA (Waveform Similarity Overlap-Add)";
    }

    const char* getDescription() const override {
        return "Fast, time-domain, optimized for voice";
    }

    bool supportsRealtime() const override { return true; }

private:
    int windowSize_;
    int hopSize_;
};

#endif // WSOLAALGORITHM_H
