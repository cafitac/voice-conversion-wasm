/**
 * SimpleTimeStretcher.h
 *
 * WSOLA (Waveform Similarity Overlap-Add) 기반 시간 늘이기/줄이기 알고리즘
 * 헤더 파일
 */

#ifndef SIMPLE_TIME_STRETCHER_H
#define SIMPLE_TIME_STRETCHER_H

#include "../audio/AudioBuffer.h"
#include "../performance/PerformanceChecker.h"
#include <vector>
#include <thread>
#include <mutex>

class SimpleTimeStretcher {
public:
    SimpleTimeStretcher();

    /**
     * 오디오의 재생 속도를 변경 (피치는 유지)
     * @param input 입력 오디오
     * @param ratio 속도 비율 (0.5 = 절반 속도, 2.0 = 2배 속도)
     * @param perfChecker 성능 측정 (optional)
     * @return 속도가 변경된 오디오
     */
    AudioBuffer process(const AudioBuffer& input, float ratio, PerformanceChecker* perfChecker = nullptr);

    /**
     * 멀티스레딩을 사용한 병렬 처리 버전
     * @param input 입력 오디오
     * @param ratio 속도 비율
     * @param numThreads 사용할 스레드 수 (0 = 자동 감지)
     * @param perfChecker 성능 측정 (optional)
     * @return 속도가 변경된 오디오
     */
    AudioBuffer processParallel(const AudioBuffer& input, float ratio,
                                int numThreads = 0, PerformanceChecker* perfChecker = nullptr);

private:
    // 파라미터들
    int sequenceMs;      // 한 조각의 길이 (밀리초)
    int seekWindowMs;    // 최적 위치를 찾을 검색 범위 (밀리초)
    int overlapMs;       // 조각들이 겹치는 길이 (밀리초)

    /**
     * 두 오디오 조각의 유사도 계산 (상관관계)
     */
    float calculateCorrelation(const float* buf1, const float* buf2, int size);

    /**
     * 검색 범위 내에서 가장 유사한 위치 찾기
     */
    int findBestOverlapPosition(const std::vector<float>& input,
                                int searchStart,
                                int searchLength,
                                const std::vector<float>& refSegment,
                                int overlapLength);

    /**
     * 두 조각을 겹쳐서 부드럽게 합치기
     */
    void overlapAndAdd(std::vector<float>& output,
                      int outputPos,
                      const std::vector<float>& input,
                      int inputPos,
                      int length,
                      float fadeIn);

    /**
     * 버퍼 용량 확보 (헬퍼 함수)
     */
    void ensureCapacity(std::vector<float>& buffer, int writePos, int additionalSize);

    /**
     * 세그먼트 복사 (헬퍼 함수)
     */
    void appendSegment(std::vector<float>& output, int& writePos,
                      const std::vector<float>& input, int inputPos, int length);
};

#endif // SIMPLE_TIME_STRETCHER_H
