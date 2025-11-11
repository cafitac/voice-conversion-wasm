#ifndef FRAMEPITCHMODIFIER_H
#define FRAMEPITCHMODIFIER_H

#include "../audio/AudioPreprocessor.h"
#include "IPitchShiftStrategy.h"
#include <vector>
#include <memory>

/**
 * FramePitchModifier
 *
 * 프레임 단위 Pitch 수정을 담당하는 클래스
 * Strategy Pattern을 사용하여 pitch shifting 알고리즘을 런타임에 교체 가능
 */
class FramePitchModifier {
public:
    /**
     * @param strategy Pitch shift 전략 (nullptr이면 기본값: HighQuality 사용)
     */
    FramePitchModifier(IPitchShiftStrategy* strategy = nullptr);
    ~FramePitchModifier();

    /**
     * 프레임별 Pitch 수정 적용
     *
     * @param frames 수정할 프레임 데이터
     * @param pitchShifts 각 프레임에 적용할 pitch shift (semitones)
     * @param sampleRate 샘플 레이트
     */
    void applyPitchShifts(
        std::vector<FrameData>& frames,
        const std::vector<float>& pitchShifts,
        int sampleRate
    );

    /**
     * Pitch shift 전략 설정 (런타임 교체)
     */
    void setStrategy(IPitchShiftStrategy* strategy);

    /**
     * 현재 사용 중인 전략 반환
     */
    IPitchShiftStrategy* getStrategy() const { return strategy_; }

private:
    IPitchShiftStrategy* strategy_;
    bool ownsStrategy_;  // 기본 전략을 내부에서 생성했는지 여부

    /**
     * RMS 재계산 (프레임 수정 후)
     */
    float calculateRMS(const std::vector<float>& samples);
};

#endif // FRAMEPITCHMODIFIER_H
