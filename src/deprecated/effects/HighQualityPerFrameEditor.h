#ifndef HIGHQUALITYPERFRAMEEDITOR_H
#define HIGHQUALITYPERFRAMEEDITOR_H

#include "../audio/AudioBuffer.h"
#include "../audio/AudioPreprocessor.h"
#include <vector>

// Forward declarations
struct PitchKeyPoint;

/**
 * TimeStretchRegion
 *
 * Duration 편집 구간을 정의
 */
struct TimeStretchRegion {
    int startFrame;  // 시작 프레임 인덱스
    int endFrame;    // 끝 프레임 인덱스
    float ratio;     // Time stretch ratio (1.5 = 1.5배 느리게)
};

/**
 * HighQualityPerFrameEditor
 *
 * 100% 자체 구현으로 프레임별 pitch/duration 편집 수행
 * - Per-frame pitch shift: Phase Vocoder 사용
 * - Region-based time stretch: WSOLA 사용
 */
class HighQualityPerFrameEditor {
public:
    HighQualityPerFrameEditor();
    ~HighQualityPerFrameEditor();

    /**
     * Key Points를 사용한 pitch 편집
     *
     * JS에서 편집한 key points를 받아서:
     * 1. Linear interpolation으로 전체 프레임의 pitch shift 계산
     * 2. Block-based processing으로 pitch shift 적용
     *
     * @param frames 전처리된 프레임 데이터
     * @param keyPoints Key points (frameIndex, semitones)
     * @param sampleRate 샘플 레이트
     * @param channels 채널 수
     * @return 편집된 오디오 버퍼
     */
    AudioBuffer applyPitchEditsWithKeyPoints(
        const std::vector<FrameData>& frames,
        const std::vector<PitchKeyPoint>& keyPoints,
        int sampleRate,
        int channels = 1
    );

    /**
     * 프레임별 pitch shift 적용
     *
     * @param frames 전처리된 프레임 데이터
     * @param pitchShiftSemitones 각 프레임의 pitch shift 값 (semitones)
     * @param sampleRate 샘플 레이트
     * @param channels 채널 수
     * @return 편집된 오디오 버퍼
     */
    AudioBuffer applyPitchEdits(
        const std::vector<FrameData>& frames,
        const std::vector<float>& pitchShiftSemitones,
        int sampleRate,
        int channels = 1
    );

    /**
     * 구간별 duration 편집 적용
     *
     * @param frames 전처리된 프레임 데이터
     * @param regions Duration 편집 구간 목록
     * @param sampleRate 샘플 레이트
     * @param channels 채널 수
     * @return 편집된 오디오 버퍼
     */
    AudioBuffer applyDurationEdits(
        const std::vector<FrameData>& frames,
        const std::vector<TimeStretchRegion>& regions,
        int sampleRate,
        int channels = 1
    );

    /**
     * Pitch + Duration 통합 편집
     *
     * @param frames 전처리된 프레임 데이터
     * @param pitchShiftSemitones 각 프레임의 pitch shift 값
     * @param regions Duration 편집 구간 목록
     * @param sampleRate 샘플 레이트
     * @param channels 채널 수
     * @return 편집된 오디오 버퍼
     */
    AudioBuffer applyAllEdits(
        const std::vector<FrameData>& frames,
        const std::vector<float>& pitchShiftSemitones,
        const std::vector<TimeStretchRegion>& regions,
        int sampleRate,
        int channels = 1
    );

private:
    /**
     * Key points를 linear interpolation하여 전체 프레임의 pitch shift 계산
     *
     * @param keyPoints Key points (frameIndex, semitones)
     * @param totalFrames 전체 프레임 수
     * @return 각 프레임의 pitch shift 값
     */
    std::vector<float> interpolateKeyPoints(
        const std::vector<PitchKeyPoint>& keyPoints,
        int totalFrames
    );

    /**
     * 단일 프레임에 Phase Vocoder pitch shift 적용
     *
     * @param frameSamples 프레임 샘플 데이터
     * @param semitones Pitch shift 값 (semitones)
     * @param sampleRate 샘플 레이트
     * @return Pitch shifted 샘플
     */
    std::vector<float> shiftFramePitch(
        const std::vector<float>& frameSamples,
        float semitones,
        int sampleRate
    );

    /**
     * 구간에 WSOLA time stretch 적용
     *
     * @param regionSamples 구간 샘플 데이터
     * @param ratio Time stretch ratio
     * @param sampleRate 샘플 레이트
     * @return Time stretched 샘플
     */
    std::vector<float> stretchRegion(
        const std::vector<float>& regionSamples,
        float ratio,
        int sampleRate
    );

    /**
     * Crossfade로 프레임 경계 부드럽게 연결
     *
     * @param buffer1 첫 번째 버퍼
     * @param buffer2 두 번째 버퍼
     * @param fadeLength Crossfade 길이
     * @return Crossfade된 샘플
     */
    std::vector<float> crossfade(
        const std::vector<float>& buffer1,
        const std::vector<float>& buffer2,
        int fadeLength
    );

    /**
     * Hanning window 생성
     */
    std::vector<float> createHanningWindow(int size);
};

#endif // HIGHQUALITYPERFRAMEEDITOR_H
