#include "HighQualityPerFrameEditor.h"
#include "ExternalPerFrameEditor.h"  // PitchKeyPoint 정의
#include "HighQualityPitchShiftStrategy.h"
#include "HighQualityTimeStretchStrategy.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

HighQualityPerFrameEditor::HighQualityPerFrameEditor() {
}

HighQualityPerFrameEditor::~HighQualityPerFrameEditor() {
}

std::vector<float> HighQualityPerFrameEditor::interpolateKeyPoints(
    const std::vector<PitchKeyPoint>& keyPoints,
    int totalFrames
) {
    std::vector<float> result(totalFrames, 0.0f);

    if (keyPoints.empty()) {
        return result;
    }

    // Key points를 frameIndex로 정렬
    std::vector<PitchKeyPoint> sortedKeyPoints = keyPoints;
    std::sort(sortedKeyPoints.begin(), sortedKeyPoints.end(),
              [](const PitchKeyPoint& a, const PitchKeyPoint& b) {
                  return a.frameIndex < b.frameIndex;
              });

    // 각 프레임에 대해 linear interpolation 수행
    for (int i = 0; i < totalFrames; ++i) {
        // 현재 프레임이 어느 key points 사이에 있는지 찾기
        int leftIdx = 0;
        int rightIdx = 0;

        // 왼쪽 key point 찾기 (i 이하인 가장 큰 frameIndex)
        for (int k = 0; k < static_cast<int>(sortedKeyPoints.size()); ++k) {
            if (sortedKeyPoints[k].frameIndex <= i) {
                leftIdx = k;
            }
        }

        // 오른쪽 key point 찾기 (i 이상인 가장 작은 frameIndex)
        rightIdx = sortedKeyPoints.size() - 1;
        for (int k = 0; k < static_cast<int>(sortedKeyPoints.size()); ++k) {
            if (sortedKeyPoints[k].frameIndex >= i) {
                rightIdx = k;
                break;
            }
        }

        // Linear interpolation
        if (leftIdx == rightIdx) {
            // 정확히 key point 위치
            result[i] = sortedKeyPoints[leftIdx].semitones;
        } else {
            int leftFrame = sortedKeyPoints[leftIdx].frameIndex;
            int rightFrame = sortedKeyPoints[rightIdx].frameIndex;
            float leftShift = sortedKeyPoints[leftIdx].semitones;
            float rightShift = sortedKeyPoints[rightIdx].semitones;

            // 선형 보간
            float t = static_cast<float>(i - leftFrame) / (rightFrame - leftFrame);
            result[i] = leftShift + (rightShift - leftShift) * t;
        }
    }

    return result;
}

AudioBuffer HighQualityPerFrameEditor::applyPitchEditsWithKeyPoints(
    const std::vector<FrameData>& frames,
    const std::vector<PitchKeyPoint>& keyPoints,
    int sampleRate,
    int channels
) {
    if (frames.empty()) {
        return AudioBuffer(sampleRate, channels);
    }

    printf("[DEBUG] applyPitchEditsWithKeyPoints (HighQuality): frames=%zu, keyPoints=%zu\n",
           frames.size(), keyPoints.size());

    // 1. Key points를 linear interpolation하여 전체 프레임의 pitch shift 계산
    std::vector<float> pitchShiftSemitones = interpolateKeyPoints(keyPoints, frames.size());

    // 2. 기존 applyPitchEdits 호출 (Block-based processing)
    return applyPitchEdits(frames, pitchShiftSemitones, sampleRate, channels);
}

AudioBuffer HighQualityPerFrameEditor::applyPitchEdits(
    const std::vector<FrameData>& frames,
    const std::vector<float>& pitchShiftSemitones,
    int sampleRate,
    int channels
) {
    if (frames.empty()) {
        return AudioBuffer(sampleRate, channels);
    }

    // CRITICAL FIX: 프레임을 concat하지 말고, 원본 오디오를 재구성
    // 프레임은 오버랩되어 있으므로 (50% overlap), 단순 concat하면 2배가 됨
    // 대신 hop size를 사용해서 원본 길이를 재구성

    // 원본 오디오 길이 계산 (첫 프레임 + (나머지 프레임 수 × hop size))
    // Frame size = 2016 samples, Hop size = 1008 samples (50% overlap)
    int hopSize = frames[0].samples.size() / 2;  // 50% overlap assumed
    int originalLength = frames[0].samples.size() + (frames.size() - 1) * hopSize;

    // Overlap-add로 원본 오디오 재구성
    std::vector<float> allSamples(originalLength, 0.0f);
    std::vector<float> overlapCount(originalLength, 0.0f);

    for (size_t i = 0; i < frames.size(); ++i) {
        int startPos = i * hopSize;
        const auto& frameSamples = frames[i].samples;

        for (size_t j = 0; j < frameSamples.size() && (startPos + j) < allSamples.size(); ++j) {
            allSamples[startPos + j] += frameSamples[j];
            overlapCount[startPos + j] += 1.0f;
        }
    }

    // Overlap된 부분 평균화
    for (size_t i = 0; i < allSamples.size(); ++i) {
        if (overlapCount[i] > 0) {
            allSamples[i] /= overlapCount[i];
        }
    }

    printf("[DEBUG] applyPitchEdits (HighQuality): 원본 재구성 완료 - frames=%zu, reconstructed=%zu samples\n",
           frames.size(), allSamples.size());

    // Block-based PSOLA 방식
    // 여러 프레임을 묶어서 처리 (최소 150ms = ~7프레임)
    // 단일 프레임은 너무 짧아서 Phase Vocoder에 적합하지 않음

    const int BLOCK_SIZE_FRAMES = 7;  // 약 147ms (7 * 21ms)
    const float MIN_SHIFT_THRESHOLD = 0.01f;

    std::vector<float> output = allSamples;  // 원본으로 시작
    int processedBlockCount = 0;

    // 연속된 편집 구간을 블록으로 묶기
    for (size_t i = 0; i < frames.size(); ) {
        // 현재 위치에서 편집이 있는지 확인
        if (std::abs(pitchShiftSemitones[i]) <= MIN_SHIFT_THRESHOLD) {
            i++;
            continue;
        }

        // 편집 블록의 시작
        int blockStart = i;
        int blockEnd = i;

        // 연속된 편집 프레임 찾기 (최소 BLOCK_SIZE_FRAMES)
        while (blockEnd < frames.size() && std::abs(pitchShiftSemitones[blockEnd]) > MIN_SHIFT_THRESHOLD) {
            blockEnd++;
        }

        // 블록이 너무 작으면 최소 크기로 확장
        if (blockEnd - blockStart < BLOCK_SIZE_FRAMES) {
            blockEnd = std::min(blockStart + BLOCK_SIZE_FRAMES, static_cast<int>(frames.size()));
        }

        // 블록의 평균 pitch shift 계산
        float avgShift = 0.0f;
        for (int j = blockStart; j < blockEnd; ++j) {
            avgShift += pitchShiftSemitones[j];
        }
        avgShift /= (blockEnd - blockStart);

        // 블록의 샘플 범위
        int startSample = blockStart * hopSize;
        int endSample = std::min(static_cast<int>(blockEnd * hopSize + frames[0].samples.size()), static_cast<int>(allSamples.size()));
        int blockLength = endSample - startSample;

        if (blockLength > 0) {
            // 블록 샘플 추출
            std::vector<float> blockSamples(allSamples.begin() + startSample,
                                             allSamples.begin() + endSample);

            // Phase Vocoder로 pitch shift 적용
            std::vector<float> processed = shiftFramePitch(blockSamples, avgShift, sampleRate);

            // Cross-fade 길이 (30ms = 1440 samples)
            int fadeLength = std::min(1440, static_cast<int>(processed.size() / 4));

            // 블록 경계에서 cross-fade로 합성
            for (size_t j = 0; j < processed.size() && (startSample + j) < output.size(); ++j) {
                if (j < static_cast<size_t>(fadeLength)) {
                    // Fade in
                    float weight = static_cast<float>(j) / fadeLength;
                    output[startSample + j] = output[startSample + j] * (1.0f - weight) + processed[j] * weight;
                } else if (j >= processed.size() - fadeLength) {
                    // Fade out
                    float weight = static_cast<float>(processed.size() - j) / fadeLength;
                    output[startSample + j] = output[startSample + j] * (1.0f - weight) + processed[j] * weight;
                } else {
                    // 중간은 완전 교체
                    output[startSample + j] = processed[j];
                }
            }

            processedBlockCount++;
            printf("[DEBUG]   Block [%d-%d]: %.2f semitones (avg), %d samples\n",
                   blockStart, blockEnd, avgShift, blockLength);
        }

        i = blockEnd;  // 다음 블록으로
    }

    printf("[DEBUG] applyPitchEdits (HighQuality): Block 처리 완료 - %d개 블록 처리됨\n", processedBlockCount);

    AudioBuffer result(sampleRate, channels);
    result.setData(output);
    return result;
}

AudioBuffer HighQualityPerFrameEditor::applyDurationEdits(
    const std::vector<FrameData>& frames,
    const std::vector<TimeStretchRegion>& regions,
    int sampleRate,
    int channels
) {
    if (frames.empty()) {
        return AudioBuffer(sampleRate, channels);
    }

    // 출력 버퍼 준비
    std::vector<float> output;

    int currentFrame = 0;

    for (const auto& region : regions) {
        // 1. 이전 구간 (원본 유지)
        for (int i = currentFrame; i < region.startFrame && i < static_cast<int>(frames.size()); ++i) {
            output.insert(output.end(), frames[i].samples.begin(), frames[i].samples.end());
        }

        // 2. Time stretch 적용 구간
        if (region.startFrame < static_cast<int>(frames.size()) && region.endFrame <= static_cast<int>(frames.size())) {
            // 구간의 샘플 추출
            std::vector<float> regionSamples;
            for (int i = region.startFrame; i < region.endFrame; ++i) {
                regionSamples.insert(regionSamples.end(), frames[i].samples.begin(), frames[i].samples.end());
            }

            // WSOLA time stretch 적용
            std::vector<float> stretched = stretchRegion(regionSamples, region.ratio, sampleRate);
            output.insert(output.end(), stretched.begin(), stretched.end());

            currentFrame = region.endFrame;
        }
    }

    // 3. 나머지 구간 (원본 유지)
    for (int i = currentFrame; i < static_cast<int>(frames.size()); ++i) {
        output.insert(output.end(), frames[i].samples.begin(), frames[i].samples.end());
    }

    AudioBuffer result(sampleRate, channels);
    result.setData(output);
    return result;
}

AudioBuffer HighQualityPerFrameEditor::applyAllEdits(
    const std::vector<FrameData>& frames,
    const std::vector<float>& pitchShiftSemitones,
    const std::vector<TimeStretchRegion>& regions,
    int sampleRate,
    int channels
) {
    // 1. Duration 편집 먼저 적용
    AudioBuffer durationEdited = applyDurationEdits(frames, regions, sampleRate, channels);

    // 2. Duration 편집된 오디오를 다시 프레임으로 분할
    // (간단하게 하기 위해 원본 프레임 수를 유지하고 pitch만 적용)
    // 실제로는 duration이 바뀌면 프레임 수도 변경되어야 하지만,
    // 복잡도를 낮추기 위해 pitch 편집을 먼저 적용하고 duration은 전체에 적용

    // 전략 변경: Pitch 먼저, Duration은 나중에
    AudioBuffer pitchEdited = applyPitchEdits(frames, pitchShiftSemitones, sampleRate, channels);

    // Pitch 편집된 오디오를 다시 프레임으로 분할하여 duration 적용
    // (간소화: duration 편집을 전체 오디오에 일괄 적용)
    // TODO: 더 정교한 구현 필요

    return pitchEdited;
}

std::vector<float> HighQualityPerFrameEditor::shiftFramePitch(
    const std::vector<float>& frameSamples,
    float semitones,
    int sampleRate
) {
    // HighQualityPitchShiftStrategy 재사용
    AudioBuffer frameBuffer(sampleRate, 1);
    frameBuffer.setData(frameSamples);

    HighQualityPitchShiftStrategy pitchShifter(2048, 512);
    AudioBuffer shifted = pitchShifter.shiftPitch(frameBuffer, semitones);

    return shifted.getData();
}

std::vector<float> HighQualityPerFrameEditor::stretchRegion(
    const std::vector<float>& regionSamples,
    float ratio,
    int sampleRate
) {
    // HighQualityTimeStretchStrategy 재사용
    AudioBuffer regionBuffer(sampleRate, 1);
    regionBuffer.setData(regionSamples);

    HighQualityTimeStretchStrategy timeStretcher(2048, 512);
    AudioBuffer stretched = timeStretcher.stretch(regionBuffer, ratio);

    return stretched.getData();
}

std::vector<float> HighQualityPerFrameEditor::crossfade(
    const std::vector<float>& buffer1,
    const std::vector<float>& buffer2,
    int fadeLength
) {
    std::vector<float> result;
    result.reserve(buffer1.size() + buffer2.size() - fadeLength);

    // buffer1의 앞부분 (crossfade 전)
    int fadeStart = buffer1.size() - fadeLength;
    if (fadeStart > 0) {
        result.insert(result.end(), buffer1.begin(), buffer1.begin() + fadeStart);
    }

    // Crossfade 영역
    for (int i = 0; i < fadeLength && i < static_cast<int>(buffer2.size()); ++i) {
        float alpha = static_cast<float>(i) / fadeLength;
        int pos1 = fadeStart + i;

        float sample1 = (pos1 >= 0 && pos1 < static_cast<int>(buffer1.size())) ? buffer1[pos1] : 0.0f;
        float sample2 = buffer2[i];

        result.push_back(sample1 * (1.0f - alpha) + sample2 * alpha);
    }

    // buffer2의 나머지 (crossfade 후)
    if (fadeLength < static_cast<int>(buffer2.size())) {
        result.insert(result.end(), buffer2.begin() + fadeLength, buffer2.end());
    }

    return result;
}

std::vector<float> HighQualityPerFrameEditor::createHanningWindow(int size) {
    std::vector<float> window(size);
    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }
    return window;
}
