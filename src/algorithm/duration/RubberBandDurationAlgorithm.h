#ifndef RUBBERBANDDURATIONALGORITHM_H
#define RUBBERBANDDURATIONALGORITHM_H

#include "IDurationAlgorithm.h"
#include "../../external/rubberband/rubberband/RubberBandStretcher.h"
#include <memory>

/**
 * RubberBandDurationAlgorithm
 *
 * RubberBand 라이브러리 기반 time stretching
 *
 * 특징:
 * - 최고 품질
 * - GPL 라이선스 (상업적 사용 주의)
 */
class RubberBandDurationAlgorithm : public IDurationAlgorithm {
public:
    RubberBandDurationAlgorithm();
    ~RubberBandDurationAlgorithm() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;

    const char* getName() const override {
        return "RubberBand Duration";
    }

    const char* getDescription() const override {
        return "Highest quality, GPL license";
    }

private:
    bool highQuality_;
};

#endif // RUBBERBANDDURATIONALGORITHM_H
