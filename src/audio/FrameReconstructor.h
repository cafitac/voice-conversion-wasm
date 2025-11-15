#ifndef FRAMERECONSTRUCTOR_H
#define FRAMERECONSTRUCTOR_H

#include "AudioBuffer.h"
#include "AudioPreprocessor.h"
#include <vector>

/**
 * FrameReconstructor
 *
 * 프레임 데이터를 Overlap-Add 방식으로 재구성하여 AudioBuffer 생성
 * Frame-based time stretching을 지원 (Wikipedia 참조)
 */
class FrameReconstructor {
public:
    FrameReconstructor();
    ~FrameReconstructor();

    /**
     * FrameData를 AudioBuffer로 재구성 (Overlap-Add 합성)
     *
     * @param frames 전처리된 프레임 데이터
     * @param sampleRate 샘플 레이트
     * @param channels 채널 수
     * @param baseHopSize 기본 프레임 간 이동 거리 (초 단위)
     * @param timeRatios 각 프레임의 time stretching ratio (비어있으면 1.0 사용)
     * @return 재구성된 오디오 버퍼
     */
    AudioBuffer reconstruct(
        const std::vector<FrameData>& frames,
        int sampleRate,
        int channels,
        float baseHopSize,
        const std::vector<float>& timeRatios = std::vector<float>()
    );

private:
    /**
     * Hanning window 생성
     */
    std::vector<float> createHanningWindow(int size);
};

#endif // FRAMERECONSTRUCTOR_H
