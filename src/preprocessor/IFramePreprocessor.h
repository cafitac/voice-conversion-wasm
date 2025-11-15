#ifndef IFRAMEPREPROCESSOR_H
#define IFRAMEPREPROCESSOR_H

#include "../audio/AudioPreprocessor.h"
#include <vector>

/**
 * IFramePreprocessor
 *
 * FrameData 전처리기 인터페이스
 * 필터 체인처럼 연결하여 사용
 *
 * 사용 예:
 *   frames → OutlierCorrector → SmoothingFilter → SplineInterpolator → result
 *
 * 모든 전처리기는 FrameData를 받아 FrameData를 반환합니다.
 */
class IFramePreprocessor {
public:
    virtual ~IFramePreprocessor() = default;

    /**
     * FrameData 전처리
     *
     * @param frames 입력 프레임들
     * @return 전처리된 프레임들
     */
    virtual std::vector<FrameData> process(
        const std::vector<FrameData>& frames
    ) = 0;

    /**
     * 전처리기 이름 반환 (디버깅/로깅용)
     */
    virtual const char* getName() const = 0;
};

#endif // IFRAMEPREPROCESSOR_H
