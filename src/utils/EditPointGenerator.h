#ifndef EDITPOINTGENERATOR_H
#define EDITPOINTGENERATOR_H

#include <vector>
#include <set>
#include "../analysis/PitchAnalyzer.h"

/**
 * EditPointGenerator
 *
 * 편집 포인트 자동 생성
 * - 균등 간격 배치 (frameInterval)
 * - 변곡점 자동 감지 (gradient threshold)
 */
class EditPointGenerator {
public:
    EditPointGenerator();
    ~EditPointGenerator();

    /**
     * 편집 포인트 생성
     *
     * @param pitchData Pitch 데이터 배열
     * @param frameInterval 기본 간격 (프레임 수)
     * @param gradientThreshold 변곡점 감지 임계값 (Hz/frame)
     * @param confidenceThreshold 최소 confidence
     * @return 편집 포인트 인덱스 배열
     */
    std::vector<int> generate(
        const std::vector<PitchPoint>& pitchData,
        int frameInterval = 10,
        float gradientThreshold = 50.0f,
        float confidenceThreshold = 0.3f
    );

private:
    /**
     * 균등 간격 포인트 추가
     */
    void addUniformPoints(
        const std::vector<PitchPoint>& pitchData,
        int frameInterval,
        float confidenceThreshold,
        std::set<int>& indices
    );

    /**
     * 변곡점 감지 및 추가
     */
    void addInflectionPoints(
        const std::vector<PitchPoint>& pitchData,
        float gradientThreshold,
        float confidenceThreshold,
        std::set<int>& indices
    );
};

#endif // EDITPOINTGENERATOR_H
