#include "EditPointManager.h"
#include "../pipeline/PitchFirstPipeline.h"

EditPointManager::EditPointManager() {
}

EditPointManager::~EditPointManager() {
}

std::vector<PitchPoint> EditPointManager::findEditablePoints(
    const std::vector<PitchPoint>& pitchData,
    float minDistance,
    float confidenceThreshold,
    int maxPoints
) {
    // confidence가 임계값 이상인 것만 필터링
    std::vector<PitchPoint> filteredData;
    for (const auto& point : pitchData) {
        if (point.confidence >= confidenceThreshold && point.frequency > 0) {
            filteredData.push_back(point);
        }
    }

    if (filteredData.size() < 3) {
        return filteredData;  // 너무 적으면 전부 반환
    }

    // Local maxima/minima 찾기
    std::vector<PitchPoint> peaks;
    float lastPeakTime = -minDistance * 2;  // 첫 포인트도 선택 가능하도록

    for (size_t i = 1; i < filteredData.size() - 1; ++i) {
        float prev = filteredData[i - 1].frequency;
        float curr = filteredData[i].frequency;
        float next = filteredData[i + 1].frequency;

        // Local maximum 또는 minimum
        bool isMax = (curr > prev) && (curr > next);
        bool isMin = (curr < prev) && (curr < next);

        if ((isMax || isMin) && (filteredData[i].time - lastPeakTime >= minDistance)) {
            peaks.push_back(filteredData[i]);
            lastPeakTime = filteredData[i].time;
        }
    }

    // 너무 많으면 균등 샘플링
    if (peaks.size() > static_cast<size_t>(maxPoints)) {
        std::vector<PitchPoint> sampledPeaks;
        int step = peaks.size() / maxPoints;
        for (size_t i = 0; i < peaks.size(); i += step) {
            sampledPeaks.push_back(peaks[i]);
            if (sampledPeaks.size() >= static_cast<size_t>(maxPoints)) {
                break;
            }
        }
        return sampledPeaks;
    }

    return peaks;
}

void EditPointManager::updateEditPoint(
    float time,
    float semitones,
    float totalDuration,
    int sampleRate,
    float gradientThreshold
) {
    // 1. 사용자 입력 저장
    editPoints_[time] = semitones;

    // 2. 부분 outlier correction 수행 (수정된 포인트 + 인접 포인트만)
    if (gradientThreshold > 0.0f && totalDuration > 0.0f) {
        correctPartialOutliers(time, gradientThreshold);
    } else {
        // outlier correction 없으면 그대로 저장
        correctedSemitones_[time] = semitones;
    }
}

void EditPointManager::correctPartialOutliers(
    float changedTime,
    float gradientThreshold
) {
    if (editPoints_.size() < 3) {
        // 포인트가 3개 미만이면 outlier detection 불가능, 그대로 사용
        for (const auto& [t, s] : editPoints_) {
            correctedSemitones_[t] = s;
        }
        return;
    }

    // 시간순 정렬된 포인트 리스트
    std::vector<std::pair<float, float>> sorted(editPoints_.begin(), editPoints_.end());
    std::sort(sorted.begin(), sorted.end());

    // 수정된 포인트의 인덱스 찾기
    int changedIdx = -1;
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (std::abs(sorted[i].first - changedTime) < 0.001f) {
            changedIdx = i;
            break;
        }
    }

    if (changedIdx == -1) {
        // 찾지 못함 (새로운 포인트)
        correctedSemitones_[changedTime] = editPoints_[changedTime];
        return;
    }

    // 영향받는 범위: 이전, 현재, 다음 포인트
    int startIdx = std::max(0, changedIdx - 1);
    int endIdx = std::min((int)sorted.size() - 1, changedIdx + 1);

    // 영향받는 포인트들만 outlier correction 수행
    for (int i = startIdx; i <= endIdx; ++i) {
        float t = sorted[i].first;
        float s = sorted[i].second;

        // 이전/다음 포인트와의 gradient 계산
        bool isOutlier = false;

        if (i > 0 && i < (int)sorted.size() - 1) {
            float prevTime = sorted[i - 1].first;
            float prevSemitones = correctedSemitones_.count(prevTime) > 0
                ? correctedSemitones_[prevTime]
                : sorted[i - 1].second;

            float nextTime = sorted[i + 1].first;
            float nextSemitones = sorted[i + 1].second;

            float dt1 = t - prevTime;
            float dt2 = nextTime - t;

            if (dt1 > 0.0f && dt2 > 0.0f) {
                float gradient1 = (s - prevSemitones) / dt1;
                float gradient2 = (nextSemitones - s) / dt2;

                // Outlier 감지: gradient 차이가 임계값 이상
                if (std::abs(gradient1 - gradient2) > gradientThreshold) {
                    isOutlier = true;
                    // 보정: 선형 보간
                    float totalDt = nextTime - prevTime;
                    float ratio = (t - prevTime) / totalDt;
                    s = prevSemitones + ratio * (nextSemitones - prevSemitones);
                }
            }
        }

        correctedSemitones_[t] = s;
    }

    // 나머지 포인트는 기존 보정값 유지 (또는 입력값 그대로)
    for (const auto& [t, s] : sorted) {
        if (correctedSemitones_.count(t) == 0) {
            correctedSemitones_[t] = s;
        }
    }
}

void EditPointManager::removeEditPoint(float time) {
    editPoints_.erase(time);
    correctedSemitones_.erase(time);
}

void EditPointManager::reset() {
    editPoints_.clear();
    correctedSemitones_.clear();
}

std::vector<FrameData> EditPointManager::getInterpolatedFrames(
    float totalDuration,
    int sampleRate,
    float gradientThreshold,
    float frameInterval
) {
    if (correctedSemitones_.empty()) {
        return std::vector<FrameData>();
    }

    // correctedSemitones_를 FrameData 배열로 변환
    // (이미 outlier correction이 적용된 값)
    std::vector<FrameData> editFrames;
    for (const auto& [time, semitones] : correctedSemitones_) {
        FrameData frame;
        frame.time = time;
        frame.pitchSemitones = semitones;
        frame.isEdited = true;
        frame.isOutlier = false;  // 이미 보정됨
        frame.isInterpolated = false;
        frame.editTime = time;  // 원본 시간 저장
        editFrames.push_back(frame);
    }

    // SplineInterpolator로 cubic spline 보간만 수행
    // (outlier correction은 이미 완료됨)
    PitchFirstPipeline pipeline(0.0f, frameInterval);  // gradientThreshold=0 (보정 안 함)
    return pipeline.preprocessOnly(editFrames, totalDuration, sampleRate);
}
