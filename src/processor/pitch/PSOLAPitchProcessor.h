#ifndef PSOLAPITCHPROCESSOR_H
#define PSOLAPITCHPROCESSOR_H

#include "IPitchProcessor.h"

/**
 * PSOLAPitchProcessor
 *
 * PSOLA (Pitch Synchronous Overlap-Add) 기반 Variable Pitch 프로세서
 *
 * 특징:
 * - 네이티브 variable pitch 지원 (각 pitch period마다 다른 shift)
 * - 빠른 처리 속도 (시간 도메인 알고리즘, FFT 불필요)
 * - 음성에 최적화
 * - 실시간 처리 가능
 *
 * 동작 원리:
 * 1. Pitch mark 검출 (autocorrelation)
 * 2. 각 pitch mark의 시간에서 FrameData의 pitchSemitones 읽기
 * 3. 해당 pitchSemitones로 grain shift
 * 4. Overlap-add로 합성
 *
 * Variable pitch의 핵심:
 *   기존: 모든 grain에 동일한 pitch scale 적용
 *   Variable: 각 grain마다 해당 시간의 pitchSemitones 사용
 *
 * 장점:
 * - 빠름 (실시간 가능)
 * - 음성에 자연스러움
 * - 작은~중간 변조량 (±4 semitones)에 우수
 *
 * 단점:
 * - 큰 변조량 (±6 semitones 이상)에서 아티팩트
 * - 무성음 처리 어려움
 */
class PSOLAPitchProcessor : public IPitchProcessor {
public:
    /**
     * @param windowSize 분석 윈도우 크기 (샘플, 기본 2048)
     * @param hopSize 홉 크기 (샘플, 기본 512)
     */
    PSOLAPitchProcessor(int windowSize = 2048, int hopSize = 512);
    ~PSOLAPitchProcessor() override;

    std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) override;

    bool supportsVariablePitch() const override { return true; }

    const char* getName() const override {
        return "PSOLA Pitch Processor";
    }

    const char* getDescription() const override {
        return "Fast, voice-optimized, native variable pitch support";
    }

private:
    int m_windowSize;
    int m_hopSize;

    /**
     * Pitch mark 검출 (autocorrelation 기반)
     *
     * @param audio 오디오 데이터
     * @param sampleRate 샘플 레이트
     * @return Pitch mark 위치들 (샘플 인덱스)
     */
    std::vector<int> detectPitchMarks(
        const std::vector<float>& audio,
        int sampleRate
    ) const;

    /**
     * Variable PSOLA Shift
     *
     * 각 pitch mark마다 다른 pitch scale 적용
     *
     * @param audio 입력 오디오
     * @param pitchMarks Pitch mark 위치들
     * @param pitchScales 각 mark의 pitch scale (size = pitchMarks.size())
     * @param sampleRate 샘플 레이트
     * @return 처리된 오디오
     */
    std::vector<float> psolaShiftVariable(
        const std::vector<float>& audio,
        const std::vector<int>& pitchMarks,
        const std::vector<float>& pitchScales,
        int sampleRate
    ) const;

    /**
     * 특정 시간의 pitchSemitones 가져오기
     *
     * FrameData에서 선형 보간하여 가져옴
     *
     * @param time 시간 (초)
     * @param frames FrameData 배열
     * @return pitchSemitones
     */
    float getPitchSemitonesAtTime(
        float time,
        const std::vector<FrameData>& frames
    ) const;

    /**
     * Hanning window 생성
     */
    std::vector<float> createHanningWindow(int size) const;

    /**
     * Pitch period 추정 (autocorrelation)
     *
     * @param audio 오디오 데이터
     * @param start 시작 위치
     * @param length 분석 길이
     * @param minPeriod 최소 period
     * @param maxPeriod 최대 period
     * @return 추정된 pitch period (샘플)
     */
    int estimatePitchPeriod(
        const std::vector<float>& audio,
        int start,
        int length,
        int minPeriod,
        int maxPeriod
    ) const;

    /**
     * FrameData를 연속 오디오로 변환
     *
     * @param frames FrameData 배열
     * @return 연속 오디오 샘플
     */
    std::vector<float> framesToAudio(const std::vector<FrameData>& frames) const;

    /**
     * 오디오를 FrameData로 변환
     *
     * @param audio 오디오 샘플
     * @param frames 원본 FrameData (메타데이터 복사용)
     * @param sampleRate 샘플 레이트
     * @return FrameData 배열
     */
    std::vector<FrameData> audioToFrames(
        const std::vector<float>& audio,
        const std::vector<FrameData>& originalFrames,
        int sampleRate
    ) const;
};

#endif // PSOLAPITCHPROCESSOR_H
