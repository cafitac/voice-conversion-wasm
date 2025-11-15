#ifndef RUBBERBANDPITCHSHIFTSTRATEGY_H
#define RUBBERBANDPITCHSHIFTSTRATEGY_H

#include "IPitchShiftStrategy.h"

/**
 * RubberBandPitchShiftStrategy
 *
 * RubberBand 라이브러리를 사용한 피치 변조 (시간은 유지)
 * - 최고 품질의 피치 변조
 * - 포먼트 보존 옵션 지원
 * - 고급 품질 옵션 (HighQuality, HighConsistency)
 * - 처리 속도는 Phase Vocoder보다 빠르고 SoundTouch보다 느림
 *
 * 주의: GPL 라이선스 (상업적 사용 시 라이선스 확인 필요)
 */
class RubberBandPitchShiftStrategy : public IPitchShiftStrategy {
public:
    /**
     * @param preserveFormant 포먼트 보존 여부 (true: 음색 유지)
     * @param highQuality 고품질 모드 (true: 최고 품질, false: 균형)
     */
    RubberBandPitchShiftStrategy(bool preserveFormant = true, bool highQuality = true);
    ~RubberBandPitchShiftStrategy() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;
    const char* getName() const override;

private:
    bool m_preserveFormant;
    bool m_highQuality;
};

#endif // RUBBERBANDPITCHSHIFTSTRATEGY_H
