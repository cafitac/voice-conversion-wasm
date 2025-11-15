#ifndef HIGHQUALITYTIMESTRETCHSTRATEGY_H
#define HIGHQUALITYTIMESTRETCHSTRATEGY_H

#include "ITimeStretchStrategy.h"
#include <vector>

/**
 * HighQualityTimeStretchStrategy
 *
 * WSOLA (Waveform Similarity Overlap-Add) 알고리즘을 사용한
 * 고품질 time stretching
 *
 * 장점:
 * - 우수한 음질
 * - Phase coherence 유지
 * - 클릭 노이즈 최소화
 * - 외부 의존성 없음
 *
 * 단점:
 * - 중간 정도의 처리 속도
 * - 복잡한 구현
 *
 * 사용 권장:
 * - 음질이 중요한 경우
 * - 외부 라이브러리를 사용할 수 없는 경우
 * - 자체 구현이 필요한 경우
 */
class HighQualityTimeStretchStrategy : public ITimeStretchStrategy {
public:
    /**
     * @param frameSize WSOLA frame 크기 (기본: 1024)
     * @param hopSize WSOLA hop 크기 (기본: 256)
     */
    HighQualityTimeStretchStrategy(int frameSize = 1024, int hopSize = 256);
    ~HighQualityTimeStretchStrategy() override;

    AudioBuffer stretch(const AudioBuffer& input, float ratio) override;
    const char* getName() const override { return "HighQualityTimeStretch (WSOLA)"; }

private:
    int frameSize_;
    int hopSize_;

    /**
     * WSOLA 알고리즘 구현 (개선 버전)
     */
    std::vector<float> wsolaStretch(const std::vector<float>& input,
                                    float ratio,
                                    int sampleRate,
                                    int channels);

    /**
     * 최적 오버랩 위치 찾기 (cross-correlation 사용)
     */
    int findBestMatch(const std::vector<float>& target,
                     const std::vector<float>& search,
                     int searchStart,
                     int searchRange);

    /**
     * Hanning window 적용
     */
    void applyWindow(std::vector<float>& frame);

    /**
     * Overlap-add with crossfade
     */
    void overlapAdd(std::vector<float>& output,
                   const std::vector<float>& frame,
                   int position,
                   int overlapSize);
};

#endif // HIGHQUALITYTIMESTRETCHSTRATEGY_H
