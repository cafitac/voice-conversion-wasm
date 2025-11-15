#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include "../src/analysis/PitchAnalyzer.h"
#include "../src/utils/EditPointGenerator.h"

/**
 * C++ 편집 파이프라인 테스트
 *
 * 테스트 시나리오:
 * 1. 가상 pitchData 생성 (sine wave 형태)
 * 2. EditPointGenerator로 편집 포인트 생성
 * 3. 편집 포인트 하나 수정
 * 4. processPitchData 로직 수행 (outlier correction + spline interpolation)
 * 5. 결과 검증
 */

// 가상 PitchData 생성 (sine wave + 급격한 변화)
std::vector<PitchPoint> generateTestPitchData(int numFrames, float duration) {
    std::vector<PitchPoint> data;

    for (int i = 0; i < numFrames; ++i) {
        PitchPoint p;
        p.time = (duration / numFrames) * i;

        // Sine wave: 200Hz ~ 300Hz
        float phase = 2.0f * M_PI * p.time / duration;
        p.frequency = 250.0f + 50.0f * std::sin(phase * 3);  // 3 cycles

        // 급격한 변화 추가 (spike)
        // Frame 25: 급상승 (300Hz → 400Hz)
        if (i == 25) {
            p.frequency = 400.0f;
        }
        // Frame 26-27: 급하강 (400Hz → 150Hz)
        else if (i == 26) {
            p.frequency = 200.0f;
        }
        else if (i == 27) {
            p.frequency = 150.0f;
        }
        // Frame 75: 또 다른 급변화 (250Hz → 350Hz)
        else if (i == 75) {
            p.frequency = 350.0f;
        }

        p.confidence = 0.8f;

        data.push_back(p);
    }

    return data;
}

// 편집 포인트 생성 (같은 로직)
std::vector<int> generateEditPointsLocal(
    const std::vector<PitchPoint>& pitchData,
    int frameInterval,
    float gradientThreshold,
    float confidenceThreshold
) {
    std::set<int> indicesSet;

    // 1. 균등 배치
    for (size_t i = 0; i < pitchData.size(); i += frameInterval) {
        if (pitchData[i].confidence >= confidenceThreshold) {
            indicesSet.insert(i);
        }
    }

    // 2. 변곡점 감지
    for (size_t i = 1; i < pitchData.size() - 1; ++i) {
        if (pitchData[i].confidence < confidenceThreshold) continue;

        float prevFreq = pitchData[i - 1].frequency;
        float currFreq = pitchData[i].frequency;
        float nextFreq = pitchData[i + 1].frequency;

        float gradient1 = std::abs(currFreq - prevFreq);
        float gradient2 = std::abs(nextFreq - currFreq);

        if (gradient1 > gradientThreshold || gradient2 > gradientThreshold) {
            indicesSet.insert(i);
            if (i > 0 && pitchData[i - 1].confidence >= confidenceThreshold) {
                indicesSet.insert(i - 1);
            }
            if (i < pitchData.size() - 1 && pitchData[i + 1].confidence >= confidenceThreshold) {
                indicesSet.insert(i + 1);
            }
        }
    }

    // 3. 마지막 포인트
    int lastIdx = pitchData.size() - 1;
    if (lastIdx >= 0 && pitchData[lastIdx].confidence >= confidenceThreshold) {
        indicesSet.insert(lastIdx);
    }

    return std::vector<int>(indicesSet.begin(), indicesSet.end());
}

// Outlier correction (같은 로직)
void correctOutlier(
    std::vector<PitchPoint>& pitchData,
    int changedIndex,
    float gradientThreshold
) {
    if (changedIndex <= 0 || changedIndex >= (int)pitchData.size() - 1) return;

    float prevFreq = pitchData[changedIndex - 1].frequency;
    float currFreq = pitchData[changedIndex].frequency;
    float nextFreq = pitchData[changedIndex + 1].frequency;

    float prevTime = pitchData[changedIndex - 1].time;
    float currTime = pitchData[changedIndex].time;
    float nextTime = pitchData[changedIndex + 1].time;

    float dt1 = currTime - prevTime;
    float dt2 = nextTime - currTime;

    if (dt1 > 0.0f && dt2 > 0.0f) {
        float prevSemitones = 12.0f * std::log2(prevFreq / 440.0f);
        float currSemitones = 12.0f * std::log2(currFreq / 440.0f);
        float nextSemitones = 12.0f * std::log2(nextFreq / 440.0f);

        float gradient1 = (currSemitones - prevSemitones) / dt1;
        float gradient2 = (nextSemitones - currSemitones) / dt2;

        if (std::abs(gradient1 - gradient2) > gradientThreshold) {
            float totalDt = nextTime - prevTime;
            float ratio = (currTime - prevTime) / totalDt;
            float correctedSemitones = prevSemitones + ratio * (nextSemitones - prevSemitones);
            float correctedFreq = 440.0f * std::pow(2.0f, correctedSemitones / 12.0f);

            std::cout << "  [Outlier Detected] Index " << changedIndex
                      << ": " << currFreq << "Hz → " << correctedFreq << "Hz\n";

            pitchData[changedIndex].frequency = correctedFreq;
        }
    }
}

// Spline interpolation (같은 로직)
void interpolateSegment(
    std::vector<PitchPoint>& pitchData,
    int changedIndex,
    const std::vector<int>& allEditIndices
) {
    std::vector<int> sortedIndices = allEditIndices;
    std::sort(sortedIndices.begin(), sortedIndices.end());

    int prevEditIdx = -1;
    int nextEditIdx = pitchData.size();

    for (int ei : sortedIndices) {
        if (ei < changedIndex && ei > prevEditIdx) prevEditIdx = ei;
        if (ei > changedIndex && ei < nextEditIdx) nextEditIdx = ei;
    }

    if (prevEditIdx == -1) prevEditIdx = 0;
    if (nextEditIdx == (int)pitchData.size()) nextEditIdx = pitchData.size() - 1;

    std::cout << "  [Interpolation] Segment: " << prevEditIdx << " → "
              << changedIndex << " → " << nextEditIdx << "\n";

    if (nextEditIdx > prevEditIdx + 1) {
        float prevFreq = pitchData[prevEditIdx].frequency;
        float nextFreq = pitchData[nextEditIdx].frequency;
        float prevSemitones = 12.0f * std::log2(prevFreq / 440.0f);
        float nextSemitones = 12.0f * std::log2(nextFreq / 440.0f);

        // Tangent 추정
        float m0 = 0.0f, m1 = 0.0f;
        if (prevEditIdx > 0) {
            float beforeFreq = pitchData[prevEditIdx - 1].frequency;
            float beforeSemitones = 12.0f * std::log2(beforeFreq / 440.0f);
            m0 = (nextSemitones - beforeSemitones) / 2.0f;
        }
        if (nextEditIdx < (int)pitchData.size() - 1) {
            float afterFreq = pitchData[nextEditIdx + 1].frequency;
            float afterSemitones = 12.0f * std::log2(afterFreq / 440.0f);
            m1 = (afterSemitones - prevSemitones) / 2.0f;
        }

        int interpolatedCount = 0;
        for (int i = prevEditIdx + 1; i < nextEditIdx; ++i) {
            if (i == changedIndex) continue;

            float t = (pitchData[i].time - pitchData[prevEditIdx].time) /
                      (pitchData[nextEditIdx].time - pitchData[prevEditIdx].time);

            float h00 = 2*t*t*t - 3*t*t + 1;
            float h10 = t*t*t - 2*t*t + t;
            float h01 = -2*t*t*t + 3*t*t;
            float h11 = t*t*t - t*t;

            float interpolatedSemitones = h00 * prevSemitones + h10 * m0 + h01 * nextSemitones + h11 * m1;
            float interpolatedFreq = 440.0f * std::pow(2.0f, interpolatedSemitones / 12.0f);

            pitchData[i].frequency = interpolatedFreq;
            interpolatedCount++;
        }

        std::cout << "  [Interpolation] " << interpolatedCount << " frames interpolated\n";
    }
}

int main() {
    std::cout << "=== C++ 편집 파이프라인 테스트 ===\n\n";

    // 1. 테스트 데이터 생성
    std::cout << "1. 테스트 데이터 생성\n";
    int numFrames = 100;
    float duration = 5.0f;
    std::vector<PitchPoint> pitchData = generateTestPitchData(numFrames, duration);
    std::cout << "  생성: " << numFrames << " frames (" << duration << "s)\n";
    std::cout << "  첫 3개: ";
    for (int i = 0; i < 3; ++i) {
        std::cout << pitchData[i].frequency << "Hz ";
    }
    std::cout << "...\n\n";

    // 2. EditPointGenerator로 편집 포인트 생성
    std::cout << "2. 편집 포인트 생성 (5프레임 단위 + 변곡점 자동 감지)\n";
    EditPointGenerator generator;
    std::vector<int> editIndices = generator.generate(pitchData, 5, 50.0f, 0.3f);
    std::cout << "  생성된 편집 포인트: " << editIndices.size() << "개\n";
    std::cout << "  인덱스: [";
    for (size_t i = 0; i < std::min(editIndices.size(), (size_t)20); ++i) {
        std::cout << editIndices[i];
        if (i < editIndices.size() - 1) std::cout << ", ";
    }
    if (editIndices.size() > 20) std::cout << ", ...";
    std::cout << "]\n";

    // 급격한 변화 지점 확인
    std::cout << "  급격한 변화 지점 포인트:\n";
    for (int idx : editIndices) {
        if ((idx >= 24 && idx <= 28) || (idx >= 74 && idx <= 76)) {
            std::cout << "    Frame " << idx << ": " << pitchData[idx].frequency << " Hz";
            if (idx == 25 || idx == 26 || idx == 27 || idx == 75) {
                std::cout << " ← 급변화";
            }
            std::cout << "\n";
        }
    }
    std::cout << "\n";

    // 3. 편집 포인트 하나 수정 (중간 포인트)
    std::cout << "3. 편집 포인트 수정\n";
    int changedIndex = editIndices[editIndices.size() / 2];
    float originalFreq = pitchData[changedIndex].frequency;
    float newFreq = originalFreq + 50.0f;  // +50Hz

    std::cout << "  Index " << changedIndex << ": " << originalFreq
              << "Hz → " << newFreq << "Hz (+50Hz)\n";
    pitchData[changedIndex].frequency = newFreq;
    std::cout << "\n";

    // 4. Outlier correction
    std::cout << "4. Outlier Correction\n";
    correctOutlier(pitchData, changedIndex, 3.0f);
    std::cout << "\n";

    // 5. Spline Interpolation
    std::cout << "5. Spline Interpolation\n";
    interpolateSegment(pitchData, changedIndex, editIndices);
    std::cout << "\n";

    // 6. 결과 검증
    std::cout << "6. 결과 검증\n";

    // 변경된 구간만 확인
    int prevEditIdx = -1, nextEditIdx = numFrames;
    for (int ei : editIndices) {
        if (ei < changedIndex && ei > prevEditIdx) prevEditIdx = ei;
        if (ei > changedIndex && ei < nextEditIdx) nextEditIdx = ei;
    }

    std::cout << "  수정된 구간: [" << prevEditIdx << " ~ " << nextEditIdx << "]\n";
    std::cout << "  구간 내 주파수:\n";

    for (int i = prevEditIdx; i <= std::min(prevEditIdx + 5, nextEditIdx); ++i) {
        std::cout << "    Frame " << std::setw(3) << i << ": "
                  << std::fixed << std::setprecision(2) << pitchData[i].frequency << " Hz";
        if (i == changedIndex) std::cout << " ← 편집됨";
        std::cout << "\n";
    }

    if (nextEditIdx - prevEditIdx > 10) {
        std::cout << "    ...\n";
        for (int i = std::max(prevEditIdx, nextEditIdx - 5); i <= nextEditIdx; ++i) {
            std::cout << "    Frame " << std::setw(3) << i << ": "
                      << std::fixed << std::setprecision(2) << pitchData[i].frequency << " Hz\n";
        }
    }

    std::cout << "\n";

    // 7. 보간 전후 비교
    std::cout << "7. 보간 효과 확인\n";
    std::vector<PitchPoint> originalData = generateTestPitchData(numFrames, duration);

    std::cout << "  구간 중간 포인트 비교:\n";
    int midIdx = (prevEditIdx + nextEditIdx) / 2;
    std::cout << "    원본:   " << originalData[midIdx].frequency << " Hz\n";
    std::cout << "    보간후: " << pitchData[midIdx].frequency << " Hz\n";
    std::cout << "    차이:   " << (pitchData[midIdx].frequency - originalData[midIdx].frequency) << " Hz\n";

    std::cout << "\n=== 테스트 완료 ===\n";

    return 0;
}
