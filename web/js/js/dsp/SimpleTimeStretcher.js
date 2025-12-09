/**
 * SimpleTimeStretcher.js
 *
 * WSOLA (Waveform Similarity Overlap-Add) 기반 시간 늘이기/줄이기 알고리즘
 * C++ 버전을 JavaScript로 완전히 동일하게 포팅
 *
 * 핵심 개념:
 * 1. 오디오를 작은 조각(segment)으로 나눔
 * 2. 조각들을 적절한 간격으로 배치 (속도에 따라 간격 조정)
 * 3. 조각들을 겹쳐서 더함 (overlap-add)
 * 4. 겹치는 부분에서 가장 유사한 위치를 찾아서 자연스럽게 연결
 */

export class SimpleTimeStretcher {
    constructor() {
        // 기본 파라미터 설정 (음악에 적합한 값들)
        this.sequenceMs = 40;      // 각 조각을 40ms로 설정
        this.seekWindowMs = 15;    // 15ms 범위 내에서 최적 위치 탐색
        this.overlapMs = 8;        // 8ms 동안 겹쳐서 합치기
    }

    /**
     * 오디오의 재생 속도를 변경 (피치는 유지)
     * @param {Float32Array} inputData - 입력 오디오 데이터
     * @param {number} sampleRate - 샘플레이트
     * @param {number} ratio - 속도 비율 (0.5 = 절반 속도, 2.0 = 2배 속도)
     * @param {PerformanceChecker} perfChecker - 성능 측정 (optional)
     * @returns {Float32Array} 속도가 변경된 오디오
     */
    process(inputData, sampleRate, ratio, perfChecker = null) {
        if (ratio <= 0) {
            console.error('[SimpleTimeStretcher] 잘못된 비율:', ratio);
            return inputData;
        }

        // 비율이 1.0에 가까우면 그냥 원본 반환
        if (Math.abs(ratio - 1.0) < 0.01) {
            return inputData;
        }

        const inputLength = inputData.length;

        // 밀리초를 샘플 수로 변환
        const sequenceSamples = Math.floor((this.sequenceMs * sampleRate) / 1000);
        const seekWindowSamples = Math.floor((this.seekWindowMs * sampleRate) / 1000);
        const overlapSamples = Math.floor((this.overlapMs * sampleRate) / 1000);

        // 출력 버퍼 크기 예측
        const estimatedOutputLength = Math.floor(inputLength / ratio) + sequenceSamples;
        const outputData = [];

        let inputPos = 0;
        let isFirstSegment = true;

        console.log('[SimpleTimeStretcher] 처리 시작 - 비율:', ratio, ', 입력 길이:', inputLength, '샘플');

        // 조각별로 처리
        while (inputPos < inputLength - sequenceSamples) {
            const segmentLength = Math.min(sequenceSamples, inputLength - inputPos);

            if (isFirstSegment) {
                // 첫 조각은 그냥 복사
                for (let i = 0; i < segmentLength; i++) {
                    outputData.push(inputData[inputPos + i]);
                }
                isFirstSegment = false;
            } else {
                // 검색 범위에서 최적 위치 찾기
                let searchStart = inputPos - seekWindowSamples;
                let searchEnd = inputPos + seekWindowSamples;

                searchStart = Math.max(0, searchStart);
                searchEnd = Math.min(inputLength - overlapSamples, searchEnd);

                // 참조용: 출력의 마지막 부분
                const refSegment = new Float32Array(overlapSamples);
                const refStart = outputData.length - overlapSamples;
                for (let i = 0; i < overlapSamples; i++) {
                    refSegment[i] = outputData[refStart + i];
                }

                // 최적 위치 찾기
                if (perfChecker) perfChecker.start('findBestOverlapPosition');
                const bestPos = this.findBestOverlapPosition(
                    inputData,
                    searchStart,
                    searchEnd - searchStart,
                    refSegment,
                    overlapSamples
                );
                if (perfChecker) perfChecker.end('findBestOverlapPosition');

                // 찾은 위치에서 조각 추출하고 겹쳐서 합치기
                if (perfChecker) perfChecker.start('overlapAndAdd');
                this.overlapAndAdd(outputData, outputData.length - overlapSamples,
                                 inputData, bestPos, overlapSamples);
                if (perfChecker) perfChecker.end('overlapAndAdd');

                // 나머지 부분 추가
                const remainingLength = sequenceSamples - overlapSamples;
                const copyStart = bestPos + overlapSamples;
                for (let i = 0; i < remainingLength && copyStart + i < inputLength; i++) {
                    outputData.push(inputData[copyStart + i]);
                }
            }

            // 다음 입력 위치 계산
            const skip = Math.floor(sequenceSamples * ratio);
            inputPos += skip;
        }

        // 남은 부분 추가
        while (inputPos < inputLength) {
            outputData.push(inputData[inputPos]);
            inputPos++;
        }

        console.log('[SimpleTimeStretcher] 처리 완료 - 출력 길이:', outputData.length, '샘플');

        return new Float32Array(outputData);
    }

    /**
     * 두 오디오 조각의 유사도 계산 (상관관계)
     */
    calculateCorrelation(buf1, buf2, size) {
        let correlation = 0.0;
        let norm1 = 0.0;
        let norm2 = 0.0;

        for (let i = 0; i < size; i++) {
            correlation += buf1[i] * buf2[i];
            norm1 += buf1[i] * buf1[i];
            norm2 += buf2[i] * buf2[i];
        }

        // 정규화
        if (norm1 > 0 && norm2 > 0) {
            correlation /= Math.sqrt(norm1 * norm2);
        }

        return correlation;
    }

    /**
     * 검색 범위 내에서 가장 유사한 위치 찾기
     */
    findBestOverlapPosition(input, searchStart, searchLength, refSegment, overlapLength) {
        let bestCorr = -1.0;
        let bestPos = searchStart;

        // 검색 범위를 돌면서 가장 유사한 위치 찾기
        for (let offset = 0; offset < searchLength - overlapLength; offset++) {
            const currentPos = searchStart + offset;

            if (currentPos + overlapLength > input.length) {
                break;
            }

            // 유사도 계산
            const inputSegment = input.subarray(currentPos, currentPos + overlapLength);
            const corr = this.calculateCorrelation(refSegment, inputSegment, overlapLength);

            // 더 유사한 위치 발견시 업데이트
            if (corr > bestCorr) {
                bestCorr = corr;
                bestPos = currentPos;
            }
        }

        return bestPos;
    }

    /**
     * 두 조각을 겹쳐서 부드럽게 합치기 (Crossfade)
     */
    overlapAndAdd(output, outputPos, input, inputPos, length) {
        for (let i = 0; i < length; i++) {
            if (outputPos + i >= output.length) {
                break;
            }
            if (inputPos + i >= input.length) {
                break;
            }

            const ratio = i / length;
            const oldSample = output[outputPos + i];
            const newSample = input[inputPos + i];

            output[outputPos + i] = oldSample * (1.0 - ratio) + newSample * ratio;
        }
    }
}
