#ifndef AUDIOPREPROCESSOR_H
#define AUDIOPREPROCESSOR_H

#include "AudioBuffer.h"
#include <vector>

// 전처리된 프레임 데이터
struct FrameData {
    // 기본 정보
    float time;                  // 시작 시간 (초)
    std::vector<float> samples;  // 프레임의 오디오 샘플들
    float rms;                   // 미리 계산된 RMS (Root Mean Square)
    bool isVoice;                // VAD 결과 (Voice Activity Detection)

    // Pitch/Duration 정보 (Variable processing용)
    float pitchSemitones;        // 이 프레임의 pitch shift 양 (semitones)
    float durationRatio;         // 이 프레임의 duration ratio (1.0 = 원본)
    float originalPitchHz;       // 원본 pitch (Hz, 검출된 값, 0 = 무성음)

    // 메타데이터 (사용자 피드백 및 디버깅용)
    bool isEdited;               // 사용자가 편집한 프레임인가?
    bool isOutlier;              // 극단값으로 보정되었는가?
    bool isInterpolated;         // 보간된 값인가?
    float editTime;              // 원본 편집 시간 (JS pitchEdits 키, isEdited=true일 때만 유효)

    // 생성자
    FrameData()
        : time(0.0f), rms(0.0f), isVoice(false),
          pitchSemitones(0.0f), durationRatio(1.0f), originalPitchHz(0.0f),
          isEdited(false), isOutlier(false), isInterpolated(false), editTime(0.0f) {}
};

/**
 * AudioPreprocessor
 *
 * 모든 Analyzer가 공통으로 사용할 전처리 수행
 * - 프레임 분할 (일관된 frameSize, hopSize)
 * - RMS 계산
 * - VAD (Voice Activity Detection)
 */
class AudioPreprocessor {
public:
    AudioPreprocessor();
    ~AudioPreprocessor();

    /**
     * 오디오 버퍼를 전처리하여 프레임 데이터로 변환
     *
     * @param buffer 원본 오디오 버퍼
     * @param frameSize 프레임 크기 (초 단위, 기본 20ms)
     * @param hopSize 프레임 간 이동 거리 (초 단위, 기본 10ms = 50% overlap)
     * @param vadThreshold VAD 임계값 (기본 0.02)
     * @return 전처리된 프레임 데이터 벡터
     */
    std::vector<FrameData> process(
        const AudioBuffer& buffer,
        float frameSize = 0.02f,
        float hopSize = 0.01f,
        float vadThreshold = 0.02f
    );

    /**
     * VAD 임계값 설정
     */
    void setVADThreshold(float threshold);

    /**
     * 노이즈 게이트 활성화 여부
     */
    void setNoiseGateEnabled(bool enabled);

private:
    float vadThreshold_;
    bool noiseGateEnabled_;

    /**
     * RMS 계산 (Root Mean Square)
     */
    float calculateRMS(const std::vector<float>& samples);

    /**
     * VAD 판단 (Voice Activity Detection)
     */
    bool detectVoice(float rms, float threshold);
};

#endif // AUDIOPREPROCESSOR_H
