#ifndef SOUNDTOUCHDURATIONPROCESSOR_H
#define SOUNDTOUCHDURATIONPROCESSOR_H

#include "IDurationProcessor.h"
#include "../../algorithm/duration/SoundTouchDurationAlgorithm.h"
#include <memory>

/**
 * SoundTouchDurationProcessor
 *
 * SoundTouch 기반 Duration 프로세서
 *
 * 특징:
 * - Frame-by-frame variable time stretch
 * - 안정적 (프로덕션 검증됨)
 * - 중간 품질
 */
class SoundTouchDurationProcessor : public IDurationProcessor {
public:
    SoundTouchDurationProcessor();
    ~SoundTouchDurationProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    const char* getName() const override {
        return "SoundTouch Duration Processor";
    }

    const char* getDescription() const override {
        return "Stable, production-tested, frame-by-frame variable duration";
    }

    bool supportsVariableDuration() const override {
        return false;  // Frame-by-frame wrapper
    }

private:
    std::unique_ptr<SoundTouchDurationAlgorithm> algorithm_;

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

#endif // SOUNDTOUCHDURATIONPROCESSOR_H
