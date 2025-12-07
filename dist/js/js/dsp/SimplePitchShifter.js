/**
 * SimplePitchShifter.js
 *
 * 피치 변경 알고리즘 (Time Stretch + Resampling 조합)
 * C++ 버전을 JavaScript로 완전히 동일하게 포팅
 *
 * 핵심 개념:
 * 1. Time Stretch로 오디오 길이를 변경 (피치도 같이 변함)
 * 2. Resampling으로 원래 길이로 되돌림 (피치만 변경됨)
 *
 * 예시:
 * - 피치를 높이려면: 먼저 느리게 만들고(stretch), 빠르게 재생(resample)
 * - 피치를 낮추려면: 먼저 빠르게 만들고(compress), 느리게 재생(resample)
 */

import { SimpleTimeStretcher } from './SimpleTimeStretcher.js';

export class SimplePitchShifter {
    constructor() {
        this.timeStretcher = new SimpleTimeStretcher();
    }

    /**
     * 오디오의 피치를 변경 (길이는 유지)
     * @param {Float32Array} inputData - 입력 오디오 데이터
     * @param {number} sampleRate - 샘플레이트
     * @param {number} semitones - 반음 단위 (-12 ~ +12, 0은 변화 없음)
     * @param {PerformanceChecker} perfChecker - 성능 측정 (optional)
     * @returns {Float32Array} 피치가 변경된 오디오
     */
    process(inputData, sampleRate, semitones, perfChecker = null) {
        // 변화가 거의 없으면 원본 반환
        if (Math.abs(semitones) < 0.01) {
            return inputData;
        }

        console.log('[SimplePitchShifter] 처리 시작 - 반음:', semitones);

        // Step 1: 반음을 비율로 변환
        if (perfChecker) perfChecker.start('semitonesToRatio');
        const pitchRatio = this.semitonesToRatio(semitones);
        if (perfChecker) perfChecker.end('semitonesToRatio');

        console.log('[SimplePitchShifter] 피치 비율:', pitchRatio);

        // Step 2: Time Stretch 적용
        // 피치를 높이려면: 먼저 느리게 (1/pitchRatio)
        // 피치를 낮추려면: 먼저 빠르게 (1/pitchRatio)
        const stretchRatio = 1.0 / pitchRatio;
        if (perfChecker) perfChecker.start('timeStretcher.process');
        const stretched = this.timeStretcher.process(inputData, sampleRate, stretchRatio, perfChecker);
        if (perfChecker) perfChecker.end('timeStretcher.process');

        console.log('[SimplePitchShifter] Time stretch 완료 - 비율:', stretchRatio);

        // Step 3: Resampling으로 원래 길이로 복원
        // 이 과정에서 피치만 변경됨
        if (perfChecker) perfChecker.start('resample');
        const result = this.resample(stretched, sampleRate, pitchRatio);
        if (perfChecker) perfChecker.end('resample');

        console.log('[SimplePitchShifter] 리샘플링 완료 - 최종 길이:', result.length, '샘플');

        return result;
    }

    /**
     * 반음(semitone)을 비율로 변환
     * 공식: ratio = 2^(semitones/12)
     *
     * 왜 2^(semitones/12)인가?
     * - 한 옥타브는 12개의 반음으로 구성
     * - 한 옥타브 위는 주파수가 2배
     * - 따라서 1개 반음당 2^(1/12) = 1.0595배
     *
     * 예시:
     * +12 반음 (한 옥타브 위) = 2^(12/12) = 2.0 (주파수 2배)
     * +7 반음 (완전5도 위) = 2^(7/12) = 1.498 (약 1.5배)
     * -12 반음 (한 옥타브 아래) = 2^(-12/12) = 0.5 (주파수 절반)
     */
    semitonesToRatio(semitones) {
        return Math.pow(2.0, semitones / 12.0);
    }

    /**
     * 리샘플링 (선형 보간 방식)
     * 오디오를 원래 길이로 되돌림
     *
     * 원리:
     * - 샘플 사이의 값을 선형으로 추정
     * - 예: [1.0, 3.0] 사이의 50% 위치 = 2.0
     */
    resample(inputData, sampleRate, ratio) {
        const inputLength = inputData.length;

        // 출력 길이 계산
        // ratio > 1.0: 더 짧아짐 (빠르게 재생)
        // ratio < 1.0: 더 길어짐 (느리게 재생)
        const outputLength = Math.floor(inputLength / ratio);

        const outputData = new Float32Array(outputLength);

        console.log('[SimplePitchShifter] 리샘플링 - 입력:', inputLength, '-> 출력:', outputLength, '샘플');

        // 각 출력 샘플마다 입력에서 값을 읽어옴
        for (let i = 0; i < outputLength; i++) {
            // 입력에서 읽을 위치 계산
            const inputPos = i * ratio;

            // 정수 부분과 소수 부분으로 분리
            const index = Math.floor(inputPos);
            const fraction = inputPos - index;

            // 범위 체크
            if (index >= inputLength - 1) {
                // 마지막 샘플 사용
                outputData[i] = inputData[inputLength - 1];
            } else {
                // 선형 보간으로 중간값 계산
                const sample1 = inputData[index];
                const sample2 = inputData[index + 1];
                outputData[i] = this.linearInterpolate(sample1, sample2, fraction);
            }
        }

        return outputData;
    }

    /**
     * 선형 보간 (Linear Interpolation)
     * 두 샘플 사이의 중간 값 계산
     *
     * 두 점 사이의 직선상에 있는 값을 계산
     * 공식: result = sample1 * (1 - fraction) + sample2 * fraction
     *
     * 예시:
     * sample1 = 1.0, sample2 = 3.0, fraction = 0.5
     * result = 1.0 * 0.5 + 3.0 * 0.5 = 2.0 (정확히 중간값)
     *
     * sample1 = 1.0, sample2 = 3.0, fraction = 0.25
     * result = 1.0 * 0.75 + 3.0 * 0.25 = 1.5 (1에 가까움)
     */
    linearInterpolate(sample1, sample2, fraction) {
        return sample1 * (1.0 - fraction) + sample2 * fraction;
    }
}
