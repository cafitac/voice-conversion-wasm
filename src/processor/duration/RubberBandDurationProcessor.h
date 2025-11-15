#ifndef RUBBERBANDDURATIONPROCESSOR_H
#define RUBBERBANDDURATIONPROCESSOR_H

#include "IDurationProcessor.h"
#include "../../algorithm/duration/RubberBandDurationAlgorithm.h"
#include <memory>

/**
 * RubberBandDurationProcessor
 *
 * RubberBand 기반 Duration 프로세서
 *
 * 특징:
 * - Frame-by-frame variable time stretch
 * - 최고 품질 (Formant preservation 지원)
 * - 음악에 최적화
 */
class RubberBandDurationProcessor : public IDurationProcessor {
public:
    RubberBandDurationProcessor(int windowSize = 2048, int hopSize = 512);
    ~RubberBandDurationProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    const char* getName() const override {
        return "RubberBand Duration Processor";
    }

    const char* getDescription() const override {
        return "Highest quality, formant-preserving, frame-by-frame variable duration";
    }

    bool supportsVariableDuration() const override {
        return false;  // Frame-by-frame wrapper
    }

private:
    std::unique_ptr<RubberBandDurationAlgorithm> algorithm_;
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

#endif // RUBBERBANDDURATIONPROCESSOR_H
