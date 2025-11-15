#ifndef SOUNDTOUCHDURATIONALGORITHM_H
#define SOUNDTOUCHDURATIONALGORITHM_H

#include "IDurationAlgorithm.h"
#include "../../external/soundtouch/include/SoundTouch.h"
#include <memory>

/**
 * SoundTouchDurationAlgorithm
 *
 * SoundTouch 라이브러리 기반 time stretching
 *
 * 특징:
 * - 검증된 안정적인 알고리즘
 * - 중간 품질
 * - LGPL 라이선스 (상업적 사용 가능)
 */
class SoundTouchDurationAlgorithm : public IDurationAlgorithm {
public:
    SoundTouchDurationAlgorithm();
    ~SoundTouchDurationAlgorithm() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;

    const char* getName() const override {
        return "SoundTouch Duration";
    }

    const char* getDescription() const override {
        return "Stable, production-tested, LGPL license";
    }

    bool supportsRealtime() const override { return true; }

private:
    std::unique_ptr<soundtouch::SoundTouch> soundTouch_;
};

#endif // SOUNDTOUCHDURATIONALGORITHM_H
