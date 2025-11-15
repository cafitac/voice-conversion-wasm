#include "EditPointGenerator.h"
#include <set>
#include <cmath>

EditPointGenerator::EditPointGenerator() {
}

EditPointGenerator::~EditPointGenerator() {
}

std::vector<int> EditPointGenerator::generate(
    const std::vector<PitchPoint>& pitchData,
    int frameInterval,
    float gradientThreshold,
    float confidenceThreshold
) {
    if (pitchData.empty()) {
        return std::vector<int>();
    }

    std::set<int> indicesSet;

    // 1. 균등 간격 포인트
    addUniformPoints(pitchData, frameInterval, confidenceThreshold, indicesSet);

    // 2. 변곡점 자동 감지
    addInflectionPoints(pitchData, gradientThreshold, confidenceThreshold, indicesSet);

    // 3. 마지막 포인트 추가
    int lastIdx = pitchData.size() - 1;
    if (lastIdx >= 0 && pitchData[lastIdx].confidence >= confidenceThreshold) {
        indicesSet.insert(lastIdx);
    }

    // 4. Set을 vector로 변환 (자동 정렬됨)
    return std::vector<int>(indicesSet.begin(), indicesSet.end());
}

void EditPointGenerator::addUniformPoints(
    const std::vector<PitchPoint>& pitchData,
    int frameInterval,
    float confidenceThreshold,
    std::set<int>& indices
) {
    for (size_t i = 0; i < pitchData.size(); i += frameInterval) {
        if (pitchData[i].confidence >= confidenceThreshold) {
            indices.insert(i);
        }
    }
}

void EditPointGenerator::addInflectionPoints(
    const std::vector<PitchPoint>& pitchData,
    float gradientThreshold,
    float confidenceThreshold,
    std::set<int>& indices
) {
    for (size_t i = 1; i < pitchData.size() - 1; ++i) {
        if (pitchData[i].confidence < confidenceThreshold) {
            continue;
        }

        float prevFreq = pitchData[i - 1].frequency;
        float currFreq = pitchData[i].frequency;
        float nextFreq = pitchData[i + 1].frequency;

        // 앞뒤 gradient 계산
        float gradient1 = std::abs(currFreq - prevFreq);
        float gradient2 = std::abs(nextFreq - currFreq);

        // 급격한 변화 감지 (변곡점)
        if (gradient1 > gradientThreshold || gradient2 > gradientThreshold) {
            indices.insert(i);

            // 변곡점 전후도 추가 (더 정확한 형태 보존)
            if (i > 0 && pitchData[i - 1].confidence >= confidenceThreshold) {
                indices.insert(i - 1);
            }
            if (i < pitchData.size() - 1 && pitchData[i + 1].confidence >= confidenceThreshold) {
                indices.insert(i + 1);
            }
        }
    }
}
