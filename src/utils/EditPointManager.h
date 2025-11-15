#ifndef EDITPOINTMANAGER_H
#define EDITPOINTMANAGER_H

#include <vector>
#include <map>
#include "../audio/AudioPreprocessor.h"
#include "../analysis/PitchAnalyzer.h"

/**
 * EditPointManager
 *
 * 편집 포인트 관리 및 보정/보간 처리
 * - 편집 가능한 포인트 찾기 (local maxima/minima)
 * - 편집 포인트 추가/수정/삭제
 * - Outlier correction + Cubic spline interpolation
 */
class EditPointManager {
public:
    EditPointManager();
    ~EditPointManager();

    /**
     * 편집 가능한 포인트 찾기 (Local maxima/minima)
     *
     * @param pitchData Pitch 데이터 배열
     * @param minDistance 최소 포인트 간격 (초)
     * @param confidenceThreshold 최소 confidence
     * @param maxPoints 최대 포인트 개수
     * @return 편집 가능한 포인트들
     */
    std::vector<PitchPoint> findEditablePoints(
        const std::vector<PitchPoint>& pitchData,
        float minDistance = 0.1f,
        float confidenceThreshold = 0.3f,
        int maxPoints = 50
    );

    /**
     * 편집 포인트 업데이트 (추가 또는 수정)
     * 부분 보간: 수정된 포인트와 인접 포인트만 재보정
     *
     * @param time 시간 (초)
     * @param semitones Pitch shift (semitones)
     * @param totalDuration 오디오 전체 길이
     * @param sampleRate 샘플레이트
     * @param gradientThreshold Outlier 감지 임계값
     */
    void updateEditPoint(
        float time,
        float semitones,
        float totalDuration = 0.0f,
        int sampleRate = 48000,
        float gradientThreshold = 3.0f
    );

    /**
     * 편집 포인트 제거
     *
     * @param time 시간 (초)
     */
    void removeEditPoint(float time);

    /**
     * 모든 편집 포인트 초기화
     */
    void reset();

    /**
     * 보정 및 보간된 전체 프레임 가져오기
     *
     * @param totalDuration 오디오 전체 길이 (초)
     * @param sampleRate 샘플레이트
     * @param gradientThreshold Outlier 감지 임계값
     * @param frameInterval 프레임 간격 (초)
     * @return 보간된 FrameData 배열
     */
    std::vector<FrameData> getInterpolatedFrames(
        float totalDuration,
        int sampleRate,
        float gradientThreshold = 3.0f,
        float frameInterval = 0.02f
    );

    /**
     * 현재 편집 포인트 개수
     */
    size_t getEditPointCount() const { return editPoints_.size(); }

    /**
     * 모든 편집 포인트 가져오기 (time, semitones) - 원본 입력값
     */
    const std::map<float, float>& getAllEditPoints() const { return editPoints_; }

    /**
     * 보정된 편집 포인트 가져오기 (time, semitones) - outlier correction 적용됨
     */
    const std::map<float, float>& getCorrectedEditPoints() const { return correctedSemitones_; }

private:
    // 편집 포인트 (time -> 사용자 입력 semitones)
    std::map<float, float> editPoints_;

    // 보정된 편집 포인트 (time -> outlier 보정된 semitones)
    std::map<float, float> correctedSemitones_;

    /**
     * 특정 포인트와 인접 포인트만 outlier correction 수행
     */
    void correctPartialOutliers(
        float changedTime,
        float gradientThreshold
    );
};

#endif // EDITPOINTMANAGER_H
