#ifndef EXTERNALPERFRAMEEDITOR_H
#define EXTERNALPERFRAMEEDITOR_H

#include "../audio/AudioBuffer.h"
#include "../audio/AudioPreprocessor.h"
#include <vector>

// TimeStretchRegion 재사용
struct TimeStretchRegion;

// Pitch Key Point 구조체 (JS에서 편집한 key points)
struct PitchKeyPoint {
    int frameIndex;      // 프레임 인덱스
    float semitones;     // Pitch shift 값 (semitones)
};

/**
 * ExternalPerFrameEditor
 *
 * Hybrid 전략: SoundTouch + 자체 알고리즘 조합
 * - SoundTouch로 전체 오디오 기본 처리
 * - 자체 알고리즘(Phase Vocoder/WSOLA)으로 미세 조정
 *
 * 전략:
 * 1. Pitch: 프레임별 평균값을 SoundTouch로 전체 적용
 * 2. Duration: 구간별로 SoundTouch 여러 번 호출하여 처리
 */
class ExternalPerFrameEditor {
public:
    ExternalPerFrameEditor();
    ~ExternalPerFrameEditor();

    /**
     * Key Points를 사용한 pitch 편집 (Hybrid)
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
     * 프레임별 pitch 편집 (Hybrid)
     *
     * 전략:
     * 1. 프레임별 pitch shift 평균값 계산
     * 2. SoundTouch로 전체 오디오에 평균값 적용
     * 3. 자체 Phase Vocoder로 각 프레임의 차이값 보정
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
     * 구간별 duration 편집 (SoundTouch)
     *
     * 전략:
     * 각 구간을 SoundTouch로 개별 처리하여 연결
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
     * Pitch + Duration 통합 편집 (Hybrid)
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
     * SoundTouch로 전체 pitch shift 적용
     *
     * @param samples 오디오 샘플
     * @param semitones Pitch shift 값 (semitones)
     * @param sampleRate 샘플 레이트
     * @return Pitch shifted 샘플
     */
    std::vector<float> applyPitchShiftSoundTouch(
        const std::vector<float>& samples,
        float semitones,
        int sampleRate
    );

    /**
     * SoundTouch로 구간 time stretch 적용
     *
     * @param samples 오디오 샘플
     * @param ratio Time stretch ratio
     * @param sampleRate 샘플 레이트
     * @return Time stretched 샘플
     */
    std::vector<float> applyTimeStretchSoundTouch(
        const std::vector<float>& samples,
        float ratio,
        int sampleRate
    );

    /**
     * 프레임별 pitch shift 평균값 계산
     *
     * @param pitchShiftSemitones 각 프레임의 pitch shift 값
     * @return 평균 pitch shift (semitones)
     */
    float calculateAveragePitchShift(const std::vector<float>& pitchShiftSemitones);

    /**
     * 프레임별 차이값 계산 (평균 대비)
     *
     * @param pitchShiftSemitones 각 프레임의 pitch shift 값
     * @param average 평균값
     * @return 각 프레임의 차이값
     */
    std::vector<float> calculatePitchDifferences(
        const std::vector<float>& pitchShiftSemitones,
        float average
    );
};

#endif // EXTERNALPERFRAMEEDITOR_H
