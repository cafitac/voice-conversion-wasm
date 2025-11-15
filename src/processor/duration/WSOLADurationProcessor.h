#ifndef WSOLADURATIONPROCESSOR_H
#define WSOLADURATIONPROCESSOR_H

#include "IDurationProcessor.h"
#include "../../algorithm/duration/WSOLAAlgorithm.h"
#include <memory>

/**
 * WSOLADurationProcessor
 *
 * WSOLA 알고리즘 기반 Duration 프로세서
 *
 * 특징:
 * - Frame-by-frame variable time stretch
 * - 빠른 처리 속도
 * - 음성에 최적화
 *
 * 동작 원리:
 * 1. FrameData의 durationRatio 읽기
 * 2. 각 구간별로 다른 time stretch ratio 적용
 * 3. Frame-by-frame으로 처리하여 variable duration 근사
 */
class WSOLADurationProcessor : public IDurationProcessor {
public:
    /**
     * @param windowSize 윈도우 크기 (기본 1024)
     * @param hopSize Hop 크기 (기본 512)
     */
    WSOLADurationProcessor(int windowSize = 1024, int hopSize = 512);
    ~WSOLADurationProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    const char* getName() const override {
        return "WSOLA Duration Processor";
    }

    const char* getDescription() const override {
        return "Fast, time-domain, frame-by-frame variable duration";
    }

    bool supportsVariableDuration() const override {
        return false;  // Frame-by-frame wrapper
    }

private:
    std::unique_ptr<WSOLAAlgorithm> algorithm_;
    int windowSize_;
    int hopSize_;

    std::vector<float> processFrameByFrame(
        const std::vector<float>& audio,
        const std::vector<FrameData>& frames,
        int sampleRate
    );

    float getDurationRatioAtTime(float time, const std::vector<FrameData>& frames) const;
    std::vector<float> framesToAudio(const std::vector<FrameData>& frames) const;
    std::vector<FrameData> audioToFrames(
        const std::vector<float>& audio,
        const std::vector<FrameData>& originalFrames,
        int sampleRate
    ) const;
};

#endif // WSOLADURATIONPROCESSOR_H
