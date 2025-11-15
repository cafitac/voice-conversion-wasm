#ifndef PSOLAPITCHSHIFTSTRATEGY_H
#define PSOLAPITCHSHIFTSTRATEGY_H

#include "IPitchShiftStrategy.h"

/**
 * PSOLAPitchShiftStrategy
 *
 * PSOLA (Pitch Synchronous Overlap-Add) 알고리즘을 사용한 피치 변조
 * - 시간 도메인 알고리즘 (FFT 불필요)
 * - 피치 마킹 기반 처리
 * - 낮은 CPU 사용량
 * - 중간 수준의 품질
 *
 * 장점:
 * - Phase Vocoder보다 빠름
 * - 실시간 처리 가능
 * - 음성에 특히 효과적
 *
 * 단점:
 * - 피치 마킹 정확도에 의존
 * - 큰 변조량에서 아티팩트 발생 가능
 */
class PSOLAPitchShiftStrategy : public IPitchShiftStrategy {
public:
    /**
     * @param windowSize 분석 윈도우 크기 (샘플)
     * @param hopSize 홉 크기 (샘플)
     */
    PSOLAPitchShiftStrategy(int windowSize = 2048, int hopSize = 512);
    ~PSOLAPitchShiftStrategy() override;

    AudioBuffer shiftPitch(const AudioBuffer& input, float semitones) override;
    const char* getName() const override;

private:
    int m_windowSize;
    int m_hopSize;

    /**
     * 피치 마킹: 각 pitch period의 위치 찾기
     *
     * @param audio 오디오 데이터
     * @param sampleRate 샘플 레이트
     * @return 피치 마크 위치들 (샘플 인덱스)
     */
    std::vector<int> detectPitchMarks(const std::vector<float>& audio, int sampleRate);

    /**
     * PSOLA를 사용한 피치 변조
     *
     * @param audio 오디오 데이터
     * @param pitchMarks 피치 마크 위치들
     * @param pitchScale 피치 배율 (2.0 = 1옥타브 위)
     * @return 변조된 오디오
     */
    std::vector<float> psolaShift(
        const std::vector<float>& audio,
        const std::vector<int>& pitchMarks,
        float pitchScale
    );

    /**
     * Hanning 윈도우 생성
     */
    std::vector<float> createHanningWindow(int size);

    /**
     * 자기상관(Autocorrelation)을 사용한 pitch period 추정
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
    );
};

#endif // PSOLAPITCHSHIFTSTRATEGY_H
