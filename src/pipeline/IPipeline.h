#ifndef IPIPELINE_H
#define IPIPELINE_H

#include "../audio/AudioPreprocessor.h"
#include "../audio/AudioBuffer.h"
#include "../preprocessor/IFramePreprocessor.h"
#include "../processor/pitch/IPitchProcessor.h"
#include "../processor/duration/IDurationProcessor.h"
#include <vector>
#include <memory>

/**
 * IPipeline
 *
 * 오디오 처리 파이프라인 인터페이스
 *
 * 파이프라인은 다음 단계를 실행합니다:
 * 1. 전처리 체인 (OutlierCorrector → SplineInterpolator)
 * 2. Pitch Processor
 * 3. Duration Processor (옵션)
 * 4. Reconstructor (FrameData → AudioBuffer)
 *
 * 두 가지 실행 모드:
 * - preprocessOnly(): 전처리만 실행하고 보간된 FrameData 반환 (그래프용)
 * - execute(): 전체 파이프라인 실행하여 AudioBuffer 반환 (재생용)
 *
 * 사용 예:
 *   // 1. 그래프 표시용 (전처리만)
 *   auto interpolatedFrames = pipeline->preprocessOnly(editPoints, sampleRate);
 *   → JavaScript로 전달하여 그래프에 표시
 *
 *   // 2. 오디오 생성용 (전체 파이프라인)
 *   auto result = pipeline->execute(frames, sampleRate, pitchProcessor, durationProcessor);
 *   → AudioBuffer를 JavaScript로 전달하여 재생
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    /**
     * 전처리만 실행 (그래프 표시용)
     *
     * @param editPoints 사용자 편집 포인트
     * @param totalDuration 전체 오디오 길이 (초)
     * @param sampleRate 샘플 레이트
     * @return 보간된 FrameData (isOutlier, isInterpolated 표시 포함)
     */
    virtual std::vector<FrameData> preprocessOnly(
        const std::vector<FrameData>& editPoints,
        float totalDuration,
        int sampleRate
    ) = 0;

    /**
     * 전체 파이프라인 실행 (오디오 생성용)
     *
     * @param audioData 원본 오디오 데이터
     * @param frames 전처리된 FrameData (preprocessOnly 결과)
     * @param sampleRate 샘플 레이트
     * @param pitchProcessor Pitch 프로세서
     * @param durationProcessor Duration 프로세서 (nullptr이면 스킵)
     * @return 처리된 AudioBuffer
     */
    virtual AudioBuffer execute(
        const std::vector<float>& audioData,
        const std::vector<FrameData>& frames,
        int sampleRate,
        IPitchProcessor* pitchProcessor,
        IDurationProcessor* durationProcessor
    ) = 0;

    /**
     * 파이프라인 이름 반환
     */
    virtual const char* getName() const = 0;

    /**
     * 파이프라인 설명 반환
     */
    virtual const char* getDescription() const = 0;

protected:
    /**
     * 전처리 체인 실행
     *
     * OutlierCorrector → SplineInterpolator
     *
     * @param frames 입력 FrameData
     * @return 전처리된 FrameData
     */
    virtual std::vector<FrameData> runPreprocessors(
        const std::vector<FrameData>& frames
    ) = 0;
};

#endif // IPIPELINE_H
